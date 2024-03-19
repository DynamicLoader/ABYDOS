
#ifndef __K_MAIN_H__
#define __K_MAIN_H__

#ifdef __cplusplus
extern "C"
{
#endif

    int k_boot(void **);
    int k_boot_perip();
    unsigned long k_boot_harts(int);

    int k_premain(int hartid);
    int k_main(int hartid);
    int k_after_main(int,int);

#ifdef __cplusplus
}
#endif

#endif