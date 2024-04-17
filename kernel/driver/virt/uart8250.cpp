/**
 * @file uart8250.cpp
 * @author DynamicLoader
 * @brief UART8250 Driver
 * @version 0.1
 * @date 2024-03-10
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <string_view>
#include <cstdio>
#include <string>
#include <cstdint>
#include "k_drvif.h"

#include "sbi/riscv_io.h"

#define UART_RBR_OFFSET 0  /* In:  Recieve Buffer Register */
#define UART_THR_OFFSET 0  /* Out: Transmitter Holding Register */
#define UART_DLL_OFFSET 0  /* Out: Divisor Latch Low */
#define UART_IER_OFFSET 1  /* I/O: Interrupt Enable Register */
#define UART_DLM_OFFSET 1  /* Out: Divisor Latch High */
#define UART_FCR_OFFSET 2  /* Out: FIFO Control Register */
#define UART_IIR_OFFSET 2  /* I/O: Interrupt Identification Register */
#define UART_LCR_OFFSET 3  /* Out: Line Control Register */
#define UART_MCR_OFFSET 4  /* Out: Modem Control Register */
#define UART_LSR_OFFSET 5  /* In:  Line Status Register */
#define UART_MSR_OFFSET 6  /* In:  Modem Status Register */
#define UART_SCR_OFFSET 7  /* I/O: Scratch Register */
#define UART_MDR1_OFFSET 8 /* I/O:  Mode Register */

#define UART_LSR_FIFOE 0x80          /* Fifo error */
#define UART_LSR_TEMT 0x40           /* Transmitter empty */
#define UART_LSR_THRE 0x20           /* Transmit-hold-register empty */
#define UART_LSR_BI 0x10             /* Break interrupt indicator */
#define UART_LSR_FE 0x08             /* Frame error indicator */
#define UART_LSR_PE 0x04             /* Parity error indicator */
#define UART_LSR_OE 0x02             /* Overrun error indicator */
#define UART_LSR_DR 0x01             /* Receiver data ready */
#define UART_LSR_BRK_ERROR_BITS 0x1E /* BI, FE, PE, OE bits */

static void drv_register();

class Drv_Uart8250 : public DriverChar
{

  private:
    struct uart8250_t
    {
        char *base;
        uint32_t freq;
        uint32_t baud;
        uint32_t reg_width;
        uint32_t reg_shift;
        // uint32_t reg_offset;

        bool opened = false;
    };

    const uint32_t default_freq = 0;
    const uint32_t default_baud = 115200;
    const uint32_t default_reg_shift = 0;
    const uint32_t default_reg_width = 1;
    const uint32_t default_reg_offset = 0;

    static u32 get_reg(const uart8250_t &u, u32 num)
    {
        u32 offset = num << u.reg_shift;

        if (u.reg_width == 1)
            return readb(u.base + offset);
        else if (u.reg_width == 2)
            return readw(u.base + offset);
        else
            return readl(u.base + offset);
    }

    static void set_reg(const uart8250_t &u, u32 num, u32 val)
    {
        u32 offset = num << u.reg_shift;

        if (u.reg_width == 1)
            writeb(val, u.base + offset);
        else if (u.reg_width == 2)
            writew(val, u.base + offset);
        else
            writel(val, u.base + offset);
    }

  public:
    int probe(const char *name, const char *compatible) override
    {
        // std::string id = name;
        // if (id.find("uart") == std::string::npos)
        //     return DRV_CAP_NONE;
        using namespace std::string_view_literals;
        return (compatible == "ns16550"sv || compatible == "ns16550a"sv || compatible == "snps,dw-apb-uart"sv) ? DRV_CAP_THIS : DRV_CAP_NONE;
    }

    long addDevice(const void *fdt, int node) override
    {
        uart8250_t uart;
        uint64_t regsize;
        int len, rc;
        const fdt32_t *val;
        rc = fdt_get_node_addr_size(fdt, node, 0, (uint64_t *)&uart.base, &regsize);

        if (rc < 0 || !uart.base || !regsize)
            return K_ENODEV;

        val = (fdt32_t *)fdt_getprop(fdt, node, "clock-frequency", &len);
        if (len > 0 && val)
            uart.freq = fdt32_to_cpu(*val);
        else
            uart.freq = default_freq;

        val = (fdt32_t *)fdt_getprop(fdt, node, "current-speed", &len);
        if (len > 0 && val)
            uart.baud = fdt32_to_cpu(*val);
        else
            uart.baud = default_baud;

        val = (fdt32_t *)fdt_getprop(fdt, node, "reg-shift", &len);
        if (len > 0 && val)
            uart.reg_shift = fdt32_to_cpu(*val);
        else
            uart.reg_shift = default_reg_shift;

        val = (fdt32_t *)fdt_getprop(fdt, node, "reg-io-width", &len);
        if (len > 0 && val)
            uart.reg_width = fdt32_to_cpu(*val);
        else
            uart.reg_width = default_reg_width;

        val = (fdt32_t *)fdt_getprop(fdt, node, "reg-offset", &len);
        if (len > 0 && val)
            uart.base += fdt32_to_cpu(*val);
        else
            uart.base += default_reg_offset;

        u16 bdiv = 0;

        if (uart.baud)
            bdiv = (uart.freq + 8 * uart.baud) / (16 * uart.baud);

        /* Disable all interrupts */
        set_reg(uart, UART_IER_OFFSET, 0x00);
        /* Enable DLAB */
        set_reg(uart, UART_LCR_OFFSET, 0x80);

        if (bdiv)
        {
            /* Set divisor low byte */
            set_reg(uart, UART_DLL_OFFSET, bdiv & 0xff);
            /* Set divisor high byte */
            set_reg(uart, UART_DLM_OFFSET, (bdiv >> 8) & 0xff);
        }

        /* 8 bits, no parity, one stop bit */
        set_reg(uart, UART_LCR_OFFSET, 0x03);
        /* Enable FIFO */
        set_reg(uart, UART_FCR_OFFSET, 0x01);
        /* No modem control DTR RTS */
        set_reg(uart, UART_MCR_OFFSET, 0x00);
        /* Clear line status */
        get_reg(uart, UART_LSR_OFFSET);
        /* Read receive buffer */
        get_reg(uart, UART_RBR_OFFSET);
        /* Set scratchpad */
        set_reg(uart, UART_SCR_OFFSET, 0x00);

        hdl.insert(std::pair<long, uart8250_t>(++hdl_count, uart));

        // Test send here
        // write(hdl_count, "TEST UART8250\n", 14);

        return hdl_count;
    }

    void removeDevice(long handler) override
    {
    }

    int open(long handler) override
    {

        auto u = hdl.find(handler);
        if (u == hdl.end())
            return K_ENODEV;

        auto &uart = u->second;
        if (uart.opened)
            return K_EALREADY;

        // lock here?
        uart.opened = true;

        printf("UART8250 opened\n");
        return 0;
    }

    int close(long handler) override
    {
        auto u = hdl.find(handler);
        if (u == hdl.end())
            return K_ENODEV;

        auto &uart = u->second;
        if (!uart.opened)
            return K_EALREADY;

        // lock here?
        uart.opened = false;

        printf("UART8250 closed\n");
        return 0;
    }

    int read(long handler, void *buf, int len) override
    {
        auto u = hdl.find(handler);
        if (u == hdl.end())
            return K_ENODEV;

        auto &uart = u->second;

        for (int i = 0; i < len; ++i)
        {
            if (get_reg(uart, UART_LSR_OFFSET) & UART_LSR_DR)
                ((char *)buf)[i] = get_reg(uart, UART_RBR_OFFSET);
            else
                return i;
        }

        // printf("UART8250 read\n");
        return len;
    }

    int write(long handler, const void *buf, int len) override
    {
        auto u = hdl.find(handler);
        if (u == hdl.end())
            return K_ENODEV;

        auto &uart = u->second;

        for (int i = 0; i < len; ++i)
        {
            while ((get_reg(uart, UART_LSR_OFFSET) & UART_LSR_THRE) == 0)
                ;

            set_reg(uart, UART_THR_OFFSET, ((const char *)buf)[i]);
        }

        // printf("UART8250 write\n");
        return len;
    }

    int ioctl(long handler, int cmd, void *arg) override
    {
        printf("UART8250 ioctl -- not supported\n");
        return 0;
    }

    // Remained devices should be closed in the destructor
    // ~Drv_Uart8250()
    // {
    //     printf("Driver destructed\n");
    // }

  private:
    int hdl_count = -1;
    std::map<long, uart8250_t> hdl;
    friend void drv_register();
};

// We make a static instance of our driver, initialize and register it
// Note that devices should be handled inside the class, not here
static DRV_INSTALL_FUNC(300) void drv_register()
{
    static Drv_Uart8250 drv;
    // drv.hdl_count = 0;
    DriverManager::addDriver(drv);
    printf("Driver UART8250 installed\n");
}