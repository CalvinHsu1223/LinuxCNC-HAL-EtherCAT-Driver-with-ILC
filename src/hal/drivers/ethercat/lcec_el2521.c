//
//    Copyright (C) 2011 Sascha Ittner <sascha.ittner@modusoft.de>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#include "hal.h"

#include "lcec.h"
#include "lcec_el2521.h"

typedef struct {
  hal_s32_t *count;		// pin: captured feedback in counts
  hal_float_t *pos_fb;		// pin: position feedback (position units)
  hal_bit_t *ramp_active;       // pin: ramp currently active
  hal_bit_t *ramp_disable;      // pin: disable ramp
  hal_bit_t *in_z;              // pin: input z
  hal_bit_t *in_z_not;          // pin: input z inverted
  hal_bit_t *in_t;              // pin: input t
  hal_bit_t *in_t_not;          // pin: input t inverted

  hal_bit_t *enable;		// pin for enable stepgen
  hal_float_t *vel_cmd;		// pin: velocity command (pos units/sec)

  hal_float_t pos_scale;	// param: steps per position unit
  hal_float_t freq;		// param: current frequency
  hal_float_t maxvel;		// param: max velocity, (pos units/sec)
  hal_float_t maxaccel_rise;	// param: max accel (pos units/sec^2)
  hal_float_t maxaccel_fall;	// param: max accel (pos units/sec^2)

  int last_operational;
  int16_t last_hw_count;	// last hw counter value
  double old_scale;		// stored scale value
  double scale_recip;		// reciprocal value used for scaling

  unsigned int state_pdo_os;
  unsigned int count_pdo_os;
  unsigned int ctrl_pdo_os;
  unsigned int freq_pdo_os;

  ec_sdo_request_t *sdo_req_base_freq;
  ec_sdo_request_t *sdo_req_max_freq;
  ec_sdo_request_t *sdo_req_ramp_rise;
  ec_sdo_request_t *sdo_req_ramp_fall;
  ec_sdo_request_t *sdo_req_ramp_factor;

  uint32_t sdo_base_freq;
  uint16_t sdo_max_freq;
  uint16_t sdo_ramp_rise;
  uint16_t sdo_ramp_fall;
  uint16_t sdo_ramp_factor;

  double freqscale;
  double freqscale_recip;
  double max_freq;
  double max_ac_rise;
  double max_ac_fall;

} lcec_el2521_data_t;

static ec_pdo_entry_info_t lcec_el2521_in[] = {
   {0x6000, 0x01, 16}, // state word
   {0x6000, 0x02, 16}  // counter value
};

static ec_pdo_entry_info_t lcec_el2521_out[] = {
   {0x7000, 0x01, 16}, // control word
   {0x7000, 0x02, 16}  // frequency value
};

static ec_pdo_info_t lcec_el2521_pdos_in[] = {
    {0x1A00, 2, lcec_el2521_in},
};

static ec_pdo_info_t lcec_el2521_pdos_out[] = {
    {0x1600, 2, lcec_el2521_out},
};

static ec_sync_info_t lcec_el2521_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL},
    {1, EC_DIR_INPUT,  0, NULL},
    {2, EC_DIR_OUTPUT, 1, lcec_el2521_pdos_out},
    {3, EC_DIR_INPUT,  1, lcec_el2521_pdos_in},
    {0xff}
};


void lcec_el2521_check_scale(lcec_el2521_data_t *hal_data);

void lcec_el2521_read(struct lcec_slave *slave, long period);
void lcec_el2521_write(struct lcec_slave *slave, long period);

int lcec_el2521_init(int comp_id, struct lcec_slave *slave, ec_pdo_entry_reg_t *pdo_entry_regs) {
  lcec_master_t *master = slave->master;
  lcec_el2521_data_t *hal_data;
  int err;
  double ramp_factor;

  // initialize callbacks
  slave->proc_read = lcec_el2521_read;
  slave->proc_write = lcec_el2521_write;

  // alloc hal memory
  if ((hal_data = hal_malloc(sizeof(lcec_el2521_data_t))) == NULL) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "hal_malloc() for slave %s.%s failed\n", master->name, slave->name);
    return -EIO;
  }
  memset(hal_data, 0, sizeof(lcec_el2521_data_t));
  slave->hal_data = hal_data;

  // read sdos
  if ((hal_data->sdo_req_base_freq = lcec_read_sdo(slave, 0x8001, 0x02, 4)) == NULL) {
    return -EIO;
  }
  hal_data->sdo_base_freq = EC_READ_U32(ecrt_sdo_request_data(hal_data->sdo_req_base_freq));
  if ((hal_data->sdo_req_ramp_rise = lcec_read_sdo(slave, 0x8001, 0x04, 2)) == NULL) {
    return -EIO;
  }
  hal_data->sdo_ramp_rise = EC_READ_U16(ecrt_sdo_request_data(hal_data->sdo_req_ramp_rise));
  if ((hal_data->sdo_req_ramp_fall = lcec_read_sdo(slave, 0x8001, 0x05, 2)) == NULL) {
    return -EIO;
  }
  hal_data->sdo_ramp_fall = EC_READ_U16(ecrt_sdo_request_data(hal_data->sdo_req_ramp_fall));
  if ((hal_data->sdo_req_ramp_factor = lcec_read_sdo(slave, 0x8000, 0x07, 2)) == NULL) {
    return -EIO;
  }
  hal_data->sdo_ramp_factor = EC_READ_U16(ecrt_sdo_request_data(hal_data->sdo_req_ramp_factor));
  if ((hal_data->sdo_req_max_freq = lcec_read_sdo(slave, 0x8800, 0x02, 2)) == NULL) {
    return -EIO;
  }
  hal_data->sdo_max_freq = EC_READ_U16(ecrt_sdo_request_data(hal_data->sdo_req_max_freq));

  // initializer sync info
  slave->sync_info = lcec_el2521_syncs;

  // initialize POD entries
  LCEC_PDO_INIT(pdo_entry_regs, slave->index, slave->vid, slave->pid, 0x6000, 0x01, &hal_data->state_pdo_os, NULL);
  LCEC_PDO_INIT(pdo_entry_regs, slave->index, slave->vid, slave->pid, 0x6000, 0x02, &hal_data->count_pdo_os, NULL);
  LCEC_PDO_INIT(pdo_entry_regs, slave->index, slave->vid, slave->pid, 0x7000, 0x01, &hal_data->ctrl_pdo_os, NULL);
  LCEC_PDO_INIT(pdo_entry_regs, slave->index, slave->vid, slave->pid, 0x7000, 0x02, &hal_data->freq_pdo_os, NULL);

  // export pins
  if ((err = hal_pin_s32_newf(HAL_OUT, &(hal_data->count), comp_id, "%s.%s.%s.stp-counts", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-counts failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_float_newf(HAL_OUT, &(hal_data->pos_fb), comp_id, "%s.%s.%s.stp-pos-fb", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-pos-fb failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_bit_newf(HAL_OUT, &(hal_data->ramp_active), comp_id, "%s.%s.%s.stp-ramp-active", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-ramp-active failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_bit_newf(HAL_IN, &(hal_data->ramp_disable), comp_id, "%s.%s.%s.stp-ramp-disable", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-ramp-disable failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_bit_newf(HAL_OUT, &(hal_data->in_z), comp_id, "%s.%s.%s.stp-in-z", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-in-z failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_bit_newf(HAL_OUT, &(hal_data->in_z_not), comp_id, "%s.%s.%s.stp-in-z-not", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-in-z-not failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_bit_newf(HAL_OUT, &(hal_data->in_t), comp_id, "%s.%s.%s.stp-in-t", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-in-t failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_bit_newf(HAL_OUT, &(hal_data->in_t_not), comp_id, "%s.%s.%s.stp-in-t-not", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-in-t-not failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_bit_newf(HAL_IN, &(hal_data->enable), comp_id, "%s.%s.%s.stp-enable", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-enable failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_float_newf(HAL_IN, &(hal_data->vel_cmd), comp_id, "%s.%s.%s.stp-velo-cmd", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-velo-cmd failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }

  // export parameters
  if ((err = hal_param_float_newf(HAL_RO, &(hal_data->freq), comp_id, "%s.%s.%s.stp-freq", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-freq failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_param_float_newf(HAL_RO, &(hal_data->maxvel), comp_id, "%s.%s.%s.stp-maxvel", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-maxvel failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_param_float_newf(HAL_RO, &(hal_data->maxaccel_fall), comp_id, "%s.%s.%s.stp-maxaccel-fall", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-maxaccel-fall failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_param_float_newf(HAL_RO, &(hal_data->maxaccel_rise), comp_id, "%s.%s.%s.stp-maxaccel-rise", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-maxaccel-rise\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_param_float_newf(HAL_RW, &(hal_data->pos_scale), comp_id, "%s.%s.%s.stp-pos-scale", LCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.stp-pos-scale failed\n", LCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }

  // set default pin values
  *(hal_data->count) = 0;
  *(hal_data->pos_fb) = 0;
  *(hal_data->ramp_active) = 0;
  *(hal_data->ramp_disable) = 0;
  *(hal_data->in_z) = 0;
  *(hal_data->in_z_not) = 0;
  *(hal_data->in_t) = 0;
  *(hal_data->in_t_not) = 0;
  *(hal_data->enable) = 0;
  *(hal_data->vel_cmd) = 0.0;

  // init parameters
  hal_data->freq = 0.0;
  hal_data->pos_scale = 1.0;
  hal_data->old_scale = 0.0;
  hal_data->scale_recip = 0.0;
  hal_data->maxvel = 0;
  hal_data->maxaccel_rise = 0;
  hal_data->maxaccel_fall = 0;

  // init other fields
  hal_data->last_operational = 0;
  hal_data->last_hw_count = 0;

  // calculate frequency factor
  if (hal_data->sdo_base_freq != 0) {
    hal_data->freqscale = (double)0x7fff / (double)hal_data->sdo_base_freq;
    hal_data->freqscale_recip = 1 / hal_data->freqscale;
  } else {
    hal_data->freqscale = 0;
    hal_data->freqscale_recip = 0;
  } 

  // calculate max frequency
  if (hal_data->sdo_max_freq != 0) {
    hal_data->max_freq = (double)hal_data->sdo_max_freq * hal_data->freqscale_recip;
  } else {
    hal_data->max_freq = (double)(hal_data->sdo_base_freq);
  }

  // calculate maximum acceleartions in Hz/s
  if ((hal_data->sdo_ramp_factor & 0x01) != 0) {
    ramp_factor = 1000;
  } else {
    ramp_factor = 10;
  }
  hal_data->max_ac_rise = ramp_factor * (double)(hal_data->sdo_ramp_rise);
  hal_data->max_ac_fall = ramp_factor * (double)(hal_data->sdo_ramp_fall);

  return 0;
}

void lcec_el2521_check_scale(lcec_el2521_data_t *hal_data) {
  // check for change in scale value
  if (hal_data->pos_scale != hal_data->old_scale) {
    // validate the new scale value
    if ((hal_data->pos_scale < 1e-20) && (hal_data->pos_scale > -1e-20)) {
      // value too small, divide by zero is a bad thing
      hal_data->pos_scale = 1.0;
    }
    // get ready to detect future scale changes
    hal_data->old_scale = hal_data->pos_scale;
    // we will need the reciprocal
    hal_data->scale_recip = 1.0 / hal_data->pos_scale;
  }
}

void lcec_el2521_read(struct lcec_slave *slave, long period) {
  lcec_master_t *master = slave->master;
  lcec_el2521_data_t *hal_data = (lcec_el2521_data_t *) slave->hal_data;
  uint8_t *pd = master->process_data;
  int16_t hw_count, hw_count_diff;
  uint16_t state;
  int in;

  // wait for slave to be operational
  if (!slave->state.operational) {
    hal_data->last_operational = 0;
    return;
  }

  // check for change in scale value
  lcec_el2521_check_scale(hal_data);

  // calculate scaled limits
  hal_data->maxvel = hal_data->max_freq * hal_data->scale_recip;
  hal_data->maxaccel_rise = hal_data->max_ac_rise * hal_data->scale_recip;
  hal_data->maxaccel_fall = hal_data->max_ac_fall * hal_data->scale_recip;

  // read state word
  state = EC_READ_U16(&pd[hal_data->state_pdo_os]);
  *(hal_data->ramp_active) = (state >> 1) & 1;
  in = (state >> 5) & 1;
  *(hal_data->in_z) = in;
  *(hal_data->in_z_not) = !in;
  in = (state >> 4) & 1;
  *(hal_data->in_t) = in;
  *(hal_data->in_t_not) = !in;

  // get counter diff
  hw_count = EC_READ_S16(&pd[hal_data->count_pdo_os]);
  hw_count_diff = hw_count - hal_data->last_hw_count;
  hal_data->last_hw_count = hw_count;
  if (!hal_data->last_operational) {
    hw_count_diff = 0;
  }

  // update raw count
  *(hal_data->count) += hw_count_diff;

  // scale position
  *(hal_data->pos_fb) = (double) (*(hal_data->count)) * hal_data->scale_recip;

  hal_data->last_operational = 1;
}

void lcec_el2521_write(struct lcec_slave *slave, long period) {
  lcec_master_t *master = slave->master;
  lcec_el2521_data_t *hal_data = (lcec_el2521_data_t *) slave->hal_data;
  uint8_t *pd = master->process_data;
  uint16_t ctrl;
  int32_t freq_raw;

  // check for change in scale value
  lcec_el2521_check_scale(hal_data);

  // write control word
  ctrl = 0;
  if (*(hal_data->ramp_disable)) {
    ctrl |= (1 << 1);
  }
  EC_WRITE_S16(&pd[hal_data->ctrl_pdo_os], ctrl);

  // update frequency
  if (*(hal_data->enable)) {
    hal_data->freq = *(hal_data->vel_cmd) * hal_data->pos_scale;
  } else {
    hal_data->freq = 0;
  }

  // output frequency
  freq_raw = hal_data->freq * hal_data->freqscale;
  if (freq_raw > 0x7fff) {
    freq_raw = 0x7fff;
  }
  if (freq_raw < -0x7fff) {
    freq_raw = -0x7fff;
  }
  EC_WRITE_S16(&pd[hal_data->freq_pdo_os], freq_raw);
}

