cmd_/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.o := gcc -Wp,-MD,/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/.dbuf.o.d  -nostdinc -isystem /usr/lib/gcc/i486-linux-gnu/4.4.3/include  -Iinclude  -I/usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include -include include/linux/autoconf.h -Iubuntu/include  -D__KERNEL__ -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -m32 -msoft-float -mregparm=3 -freg-struct-return -mpreferred-stack-boundary=2 -march=i586 -mtune=generic -maccumulate-outgoing-args -Wa,-mtune=generic32 -ffreestanding -fstack-protector -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -pipe -Wno-sign-compare -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -Wframe-larger-than=1024 -fno-omit-frame-pointer -fno-optimize-sibling-calls -pg -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fno-dwarf2-cfi-asm -fconserve-stack -Os -I. -I/usr/realtime-2.6.32-122-rtai/include -I. -I/usr/realtime-2.6.32-122-rtai/include -D_FORTIFY_SOURCE=0 -mhard-float -DRTAI=3 -DRTAPI -D_GNU_SOURCE -Drealtime -D_FORTIFY_SOURCE=0 -D__MODULE__ -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/libnml/linklist -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/libnml/cms -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/libnml/rcs -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/libnml/inifile -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/libnml/os_intf -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/libnml/nml -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/libnml/buffer -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/libnml/posemath -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/rtapi -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/hal -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/nml_intf -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/kinematics -I/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion -DSEQUENTIAL_SUPPORT -DHAL_SUPPORT -DDYNAMIC_PLCSIZE -DRT_SUPPORT -DOLD_TIMERS_MONOS_SUPPORT -DMODBUS_IO_MASTER -fno-fast-math -mieee-fp -fno-unsafe-math-optimizations -Wframe-larger-than=2560  -DMODULE -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(dbuf)"  -D"KBUILD_MODNAME=KBUILD_STR(motmod)"  -c -o /home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.o /home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.c

deps_/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.o := \
  /home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.c \
  /home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/rtapi/rtapi_string.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/types.h \
    $(wildcard include/config/x86/64.h) \
    $(wildcard include/config/highmem64g.h) \
  include/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/posix_types.h \
    $(wildcard include/config/x86/32.h) \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/posix_types_32.h \
  /usr/lib/gcc/i486-linux-gnu/4.4.3/include/stdarg.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/string.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/string_32.h \
    $(wildcard include/config/x86/use/3dnow.h) \
    $(wildcard include/config/kmemcheck.h) \
  include/linux/version.h \
  /home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/rtapi/rtapi_errno.h \
  include/linux/errno.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/errno.h \
  include/asm-generic/errno.h \
  include/asm-generic/errno-base.h \
  /home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.h \

/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.o: $(deps_/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.o)

$(deps_/home/yutingkao23/ILC-with-LinuxCNC-ILCv1/src/emc/motion/dbuf.o):
