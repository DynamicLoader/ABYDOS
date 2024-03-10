#ifndef __K_SBIF_H__
#define __K_SBIF_H__

extern "C"
{
#include "llenv.h"
// #include "riscv_asm.h"
// #include "sbi/riscv_encoding.h"

#include <cstring>

// #undef OPENSBI_EXTERNAL_SBI_TYPES

} // extern "C"

using sbiret_t = struct sbiret;

class SBIF
{
  private:
    static sbiret_t callSBI(int ext, int fid, unsigned long arg0 = 0, unsigned long arg1 = 0, unsigned long arg2 = 0,
                            unsigned long arg3 = 0, unsigned long arg4 = 0, unsigned long arg5 = 0)
    {
        return sbi_ecall(ext, fid, arg0, arg1, arg2, arg3, arg4, arg5);
    }

    template <unsigned long EXTID> class impl_helper
    {
      protected:
        static sbiret_t CSBI(int fid, unsigned long arg0 = 0, unsigned long arg1 = 0, unsigned long arg2 = 0,
                             unsigned long arg3 = 0, unsigned long arg4 = 0, unsigned long arg5 = 0)
        {
            return sbi_ecall(EXTID, fid, arg0, arg1, arg2, arg3, arg4, arg5);
        }
    };

  public:
    class Base
    {
      public:
        static unsigned int getSpecVersion()
        {
            auto ret = callSBI(SBI_EXT_BASE, SBI_EXT_BASE_GET_SPEC_VERSION);
            return ret.value & 0x7FFFFFFF;
        }

        static auto getImplID()
        {
            return callSBI(SBI_EXT_BASE, SBI_EXT_BASE_GET_IMP_ID).value;
        }

        static auto getImplVer()
        {
            return callSBI(SBI_EXT_BASE, SBI_EXT_BASE_GET_IMP_VERSION).value;
        }

        static auto probeExtension(long extid)
        {
            return callSBI(SBI_EXT_BASE, SBI_EXT_BASE_PROBE_EXT).value;
        }

        static auto getVID()
        {
            return callSBI(SBI_EXT_BASE, SBI_EXT_BASE_GET_MVENDORID).value;
        }

        static auto getAID()
        {
            return callSBI(SBI_EXT_BASE, SBI_EXT_BASE_GET_MARCHID).value;
        }

        static auto getMacImplID()
        {
            return callSBI(SBI_EXT_BASE, SBI_EXT_BASE_GET_MIMPID).value;
        }

        static const char *getImplName(unsigned long id)
        {
            static const char *impls[] = {"BBL",    "OpenSBI", "Xvisor",     "KVM",      "RustSBI",
                                          "Diosix", "Coffer",  "XenProject", "PolarFire"};
            return (id >= 0 and id < 9) ? impls[id] : "";
        }
    };

    class DebugCon : public impl_helper<SBI_EXT_DBCN>
    {
      public:
        static auto puts(const char *str)
        {
            return CSBI(SBI_EXT_DBCN_CONSOLE_WRITE, strlen(str), (unsigned long)str).error;
        }

        static auto gets(char *str, unsigned long mlen)
        {
            auto ret = CSBI(SBI_EXT_DBCN_CONSOLE_READ, mlen, (unsigned long)str);
            return (ret.error == SBI_SUCCESS ? ret.value : ret.error);
        }
    };

    class Timer : public impl_helper<SBI_EXT_TIME>
    {
      public:
        static auto setTimer(unsigned long sv)
        {
            return CSBI(SBI_EXT_TIME_SET_TIMER, sv).error;
        }

        static auto clearTimer()
        {
            return CSBI(SBI_EXT_TIME_SET_TIMER, (0xFFFFFFFFFFFFFFFF) - 1).error;
        }
    };

    class IPI : public impl_helper<SBI_EXT_IPI>
    {
      public:
        // Always Success
        static void sendIPI(unsigned long hart_mask, unsigned long event)
        {
            CSBI(SBI_EXT_IPI_SEND_IPI, hart_mask, event);
        }
    };

    class HSM : public impl_helper<SBI_EXT_HSM>
    {
      public:
        static long getHartState(unsigned long hartid)
        {
            auto ret = CSBI(SBI_EXT_HSM_HART_GET_STATUS, hartid);
            return (ret.error ? ret.error : ret.value);
        }

        static auto startHart(unsigned long hartid, unsigned long start_addr, unsigned long arg)
        {
            return CSBI(SBI_EXT_HSM_HART_START, hartid, start_addr, arg).error;
        }

        static auto stopHart()
        {
            return CSBI(SBI_EXT_HSM_HART_STOP).error;
        }

        static auto suspendHart(unsigned int suspend_type, unsigned long resume_addr, unsigned long arg)
        {
            return CSBI(SBI_EXT_HSM_HART_SUSPEND, suspend_type, resume_addr, arg).error;
        }
    };

    class SRST : public impl_helper<SBI_EXT_SRST>
    {
      public:
        enum ResetType
        {
            Shutdown = 0,
            ColdReboot,
            WarmReboot
        };

        enum ResetReason
        {
            NoReason = 0,
            SystemFailure
        };
        static auto reset(ResetType reset_type, ResetReason reason)
        {
            return CSBI(SBI_EXT_SRST_RESET, reset_type, reason).error;
        }
    };

    static const char *getErrorStr(long err)
    {
        const char *err_str[] = {"SBI_SUCCESS",
                                 "SBI_ERR_FAILED",
                                 "SBI_ERR_NOT_SUPPORTED",
                                 "SBI_ERR_INVALID_PARAM",
                                 "SBI_ERR_DENIED",
                                 "SBI_ERR_INVALID_ADDRESS",
                                 "SBI_ERR_ALREADY_AVAILABLE",
                                 "SBI_ERR_ALREADY_STARTED",
                                 "SBI_ERR_ALREADY_STOPPED",
                                 "SBI_ERR_NO_SHMEM"};
        if (err >= 0)
            return err_str[0];
        else if (err < 0 and err > -10)
            return err_str[-err];
        else
            return "SBI_ERR_UNKNOWN";
    }
};
#endif