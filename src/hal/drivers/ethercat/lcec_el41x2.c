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

#include "lcec.h"
#include "lcec_el41x2.h"

typedef struct {
  hal_bit_t *pos;
  hal_bit_t *neg;
  hal_bit_t *enable;
  hal_bit_t *absmode;
  hal_float_t *value;
  hal_float_t *scale;
  hal_float_t *offset;
  double old_scale;
  double scale_recip;
  hal_float_t *min_dc;
  hal_float_t *max_dc;
  hal_float_t *curr_dc;
  hal_s32_t *raw_val;
  unsigned int val_pdo_os;
} lcec_el41x2_chan_t;

typedef struct {
  lcec_el41x2_chan_t chans[LCEC_EL41x2_CHANS];
} lcec_el41x2_data_t;

static ec_pdo_entry_info_t lcec_el41x2_channel1[] = {
    {0x3001, 1, 16}  // output
};

static ec_pdo_entry_info_t lcec_el41x2_channel2[] = {
    {0x3002, 1, 16}  // output
};

static ec_pdo_info_t lcec_el41x2_pdos_in[] = {
    {0x1600, 1, lcec_el41x2_channel1},
    {0x1601, 1, lcec_el41x2_channel2}
};

static ec_sync_info_t lcec_el41x2_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL},
    {1, EC_DIR_INPUT,  0, NULL},
    {2, EC_DIR_OUTPUT, 2, lcec_el41x2_pdos_in},
    {0xff}
};

void lcec_el41x2_write(struct lcec_slave *slave, long period);

int lcec_el41x2_init(int comp_id, struct lcec_slave *slave, ec_pdo_entry_reg_t *pdo_entry_regs) {
  lcec_master_t *master = slave->master;
  lcec_el41x2_data_t *hal_data;
  lcec_el41x2_chan_t *chan;
  int i;
  int err;

  // initialize callbacks
  slave->proc_write = lcec_el41x2_write;

  // alloc hal memory
  if ((hal_data = hal_malloc(sizeof(lcec_el41x2_data_t))) == NULL) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "hal_malloc() for slave %s.%s failed\n", master->name, slave->name);
    return -EIO;
  }
  memset(hal_data, 0, sizeof(lcec_el41x2_data_t));
  slave->hal_data = hal_data;

  // initializer sync info
  slave->sync_info = lcec_el41x2_syncs;

  // initialize pins
  for (i=0; i<LCEC_EL41x2_CHANS; i++) {
    chan = &hal_data->chans[i];

    // initialize POD entries
    LCEC_PDO_INIT(pdo_entry_regs, slave->index, slave->vid, slave->pid, 0x3001 + i, 0x01, &chan->val_pdo_os, NULL);

    // export paramameters
    if ((err = hal_pin_float_newf(HAL_IO, &(chan->scale), comp_id, "%s.%s.%s.aout-%d-scale", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-scale failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }
    if ((err = hal_pin_float_newf(HAL_IO, &(chan->offset), comp_id, "%s.%s.%s.aout-%d-offset", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-offset failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }
    if ((err = hal_pin_float_newf(HAL_IO, &(chan->min_dc), comp_id, "%s.%s.%s.aout-%d-min-dc", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-min-dc failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }
    if ((err = hal_pin_float_newf(HAL_IO, &(chan->max_dc), comp_id, "%s.%s.%s.aout-%d-max-dc", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-max-dc failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }
    if ((err = hal_pin_float_newf(HAL_OUT, &(chan->curr_dc), comp_id, "%s.%s.%s.aout-%d-curr-dc", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-curr-dc failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }

    // export pins
    if ((err = hal_pin_bit_newf(HAL_IN, &(chan->enable), comp_id, "%s.%s.%s.aout-%d-enable", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-enable failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }
    if ((err = hal_pin_bit_newf(HAL_IN, &(chan->absmode), comp_id, "%s.%s.%s.aout-%d-absmode", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-absmode failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }
    if ((err = hal_pin_float_newf(HAL_IN, &(chan->value), comp_id, "%s.%s.%s.aout-%d-value", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-value failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }
    if ((err = hal_pin_s32_newf(HAL_OUT, &(chan->raw_val), comp_id, "%s.%s.%s.aout-%d-raw", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-raw failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }

    // export UP/DOWN pins
    if ((err = hal_pin_bit_newf(HAL_OUT, &(chan->pos), comp_id, "%s.%s.%s.aout-%d-pos", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-pos failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }
    if ((err = hal_pin_bit_newf(HAL_OUT, &(chan->neg), comp_id, "%s.%s.%s.aout-%d-neg", LCEC_MODULE_NAME, master->name, slave->name, i)) != 0) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.aout-%d-neg failed\n", LCEC_MODULE_NAME, master->name, slave->name, i);
      return err;
    }

    // set default pin values
    *(chan->scale) = 1.0;
    *(chan->offset) = 0.0;
    *(chan->min_dc) = -1.0;
    *(chan->max_dc) = 1.0;
    *(chan->curr_dc) = 0.0;
    *(chan->enable) = 0;
    *(chan->absmode) = 0;
    *(chan->value) = 0.0;
    *(chan->raw_val) = 0.0;
    *(chan->pos) = 0;
    *(chan->neg) = 0;

    // init other fields
    chan->old_scale = *(chan->scale) + 1.0;
    chan->scale_recip = 1.0;
  }

  return 0;
}

void lcec_el41x2_write(struct lcec_slave *slave, long period) {
  lcec_master_t *master = slave->master;
  lcec_el41x2_data_t *hal_data = (lcec_el41x2_data_t *) slave->hal_data;
  uint8_t *pd = master->process_data;
  int i;
  lcec_el41x2_chan_t *chan;
  double tmpval, tmpdc, raw_val;

  // check inputs
  for (i=0; i<LCEC_EL41x2_CHANS; i++) {
    chan = &hal_data->chans[i];

    // validate duty cycle limits, both limits must be between
    // 0.0 and 1.0 (inclusive) and max must be greater then min
    if (*(chan->max_dc) > 1.0) {
      *(chan->max_dc) = 1.0;
    }
    if (*(chan->min_dc) > *(chan->max_dc)) {
      *(chan->min_dc) = *(chan->max_dc);
    }
    if (*(chan->min_dc) < -1.0) {
      *(chan->min_dc) = -1.0;
    }
    if (*(chan->max_dc) < *(chan->min_dc)) {
      *(chan->max_dc) = *(chan->min_dc);
    }

    // do scale calcs only when scale changes
    if (*(chan->scale) != chan->old_scale) {
      // validate the new scale value
      if ((*(chan->scale) < 1e-20) && (*(chan->scale) > -1e-20)) {
        // value too small, divide by zero is a bad thing
        *(chan->scale) = 1.0;
      }
      // get ready to detect future scale changes
      chan->old_scale = *(chan->scale);
      // we will need the reciprocal
      chan->scale_recip = 1.0 / *(chan->scale);
    }

    // get command
    tmpval = *(chan->value);
    if (*(chan->absmode) && (tmpval < 0)) {
      tmpval = -tmpval;
    }

    // convert value command to duty cycle
    tmpdc = tmpval * chan->scale_recip + *(chan->offset);
    if (tmpdc < *(chan->min_dc)) {
      tmpdc = *(chan->min_dc);
    }
    if (tmpdc > *(chan->max_dc)) {
      tmpdc = *(chan->max_dc);
    }

    // set output values
    if (*(chan->enable) == 0) {
      raw_val = 0;
      *(chan->pos) = 0;
      *(chan->neg) = 0;
      *(chan->curr_dc) = 0;
    } else {
      raw_val = (double)0x7fff * tmpdc;
      if (raw_val > (double)0x7fff) {
        raw_val = (double)0x7fff;
      }
      if (raw_val < (double)-0x7fff) {
        raw_val = (double)-0x7fff;
      }
      *(chan->pos) = (*(chan->value) > 0);
      *(chan->neg) = (*(chan->value) < 0);
      *(chan->curr_dc) = tmpdc;
    }

    // update value
    EC_WRITE_S16(&pd[chan->val_pdo_os], (int16_t)raw_val);
    *(chan->raw_val) = (int32_t)raw_val;
  }
}

