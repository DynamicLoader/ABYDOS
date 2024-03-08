
#ifndef __SBIF_H__
#define __SBIF_H__

#include <sbi/sbi_ecall_interface.h>

struct sbiret {
  unsigned long error;
  unsigned long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                        unsigned long arg1, unsigned long arg2,
                        unsigned long arg3, unsigned long arg4,
                        unsigned long arg5);

#endif // SBIF_H