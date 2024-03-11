# ABYDOS

AbydOS is a brand new operating system targeting RISC-V64, mainly based on C++.

## Features

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

The package from Ubuntu/Debian does not contain newlib or libstdc++. Therefore, it is required to use a full toolchain that supports them. Toolchains from [riscv-collab](https://github.com/riscv-collab/riscv-gnu-toolchain) are compiled with -mcmodel=medlow by default, which is not compatible with 0x80000000 as the kernel base. I have forked and compiled it with -mcmodel=medany [here](https://github.com/DynamicLoader/riscv-gnu-toolchain). If you need it, you can go to the actions for downloading.

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

It's not necessary to build OpenSBI for a full installation of QEMU (-bios=default). if you don't, please compile it with `PLATFORM=generic` then run the QEMU using the command in under `opensbi` directory:

```bash
qemu-system-riscv64 -M virt -m 256M -nographic -bios build/platform/generic/firmware/fw_jump.elf -kernel ../build/AbydOS_KNL
```

To debug it, start the QEMU with gdb-stub like:

```bash
qemu-system-riscv64 -M virt -m 256M -nographic -bios build/platform/generic/firmware/fw_jump.elf -kernel ../build/AbydOS_KNL -gdb tcp::1234 -S
```
This will suspend until gdb attach and instruct it to continue; then in the secondary terminal, use:

```bash
gdb-multiarch ../build/AbydOS_KNL -ex 'target remote 127.0.0.1:1234'
```

to attach and debug.

## License

Under BSD-3.