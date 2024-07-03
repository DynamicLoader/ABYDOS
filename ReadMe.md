# ABYDOS

AbydOS is a brand new operating system targeting RISC-V64, mainly based on C++.

## Features

- Multi-hart support
- SV39/SV48/SV57 MMU support
- DeviceTree-based device probing
- Under development...

## Dependencies

- [OpenSBI](https://github.com/riscv-software-src/opensbi)
- [DTC](https://git.kernel.org/pub/scm/utils/dtc/dtc.git)

## Build

To build this project, you need a Linux platform and a cross-compiling toolchain.

Currently tested on Ubuntu 20.04, with `gcc-riscv64-unknown-elf` and `qemu-system`.

After installing the necessary packages, clone the repo and initialize the submodules.

Then use CMake to configure and build. If everything goes well, you should get the kernel ELF `AbydOS_KNL` and its binary.

**About the Toolchain**:

The package from Ubuntu/Debian does not contain newlib or libstdc++. Therefore, it is required to use a full toolchain that supports them. Toolchains from [riscv-collab](https://github.com/riscv-collab/riscv-gnu-toolchain) are compiled with -mcmodel=medlow by default, which is not compatible with 0x80000000 as the kernel base. I have forked and compiled it with -mcmodel=medany (also other options to get the kernel work) [here](https://github.com/dynamicloader/riscv-gnu-toolchain). Just go to the [Actions] for downloading.

**Notes on configuring CMake:**

Since `riscv64-unknown-elf-` does not target a normal system, it cannot pass the compiler test by CMake. Therefore, it is important to add these two command-line arguments to skip the test:

```
-DCMAKE_C_COMPILER_WORKS:STRING=1 
-DCMAKE_CXX_COMPILER_WORKS:STRING=1
```

In VSCODE, it can be done by editing the `settings.json` and add the segment below:

```json
    "cmake.configureSettings": {
        "CMAKE_C_COMPILER_WORKS": 1,
        "CMAKE_CXX_COMPILER_WORKS": 1
    }
```

## Testing

It's not necessary to build OpenSBI for a full installation of QEMU (-bios=default). (But for now we'd use 0x80100000 as kernel start, which does NOT compatible with default 0x80200000) If you don't, please compile it with `PLATFORM=generic FW_JUMP_OFFSET=0x80100000` then run the QEMU using the command in the project directory:

```bash
qemu-system-riscv64 -M virt -m 256M -nographic -bios build/opensbi/build/platform/generic/firmware/fw_jump.elf -kernel build/AbydOS_KNL
```

Multi-hart is supported, so `-smp 8` can be added to test. 

If you'd like to connect a FLASH, just generate a flash file with exact size of 32MB, then add the arguement below:

```
-drive if=pflash,file=/path/to/pflash,unit=1
```

Note that `unit=1` is required since QEMU takes the first pflash as a boot device.

To debug it, start the QEMU with gdb-stub like:

```bash
qemu-system-riscv64 -M virt -m 256M -nographic -bios build/opensbi/build/platform/generic/firmware/fw_jump.elf -kernel build/AbydOS_KNL -gdb tcp::1234 -S
```
This will suspend until gdb attach and instruct it to continue; then in the secondary terminal, use:

```bash
gdb-multiarch build/AbydOS_KNL -ex 'target remote 127.0.0.1:1234'
```

to attach and debug.

## License

Under BSD-3.