# ABYDOS

AbydOS is a brand new operating system targeting RISC-V64, mainly based on C++.

## Features

- Under development...

## Dependencies

- [OpenSBI](https://github.com/riscv-software-src/opensbi)
- [ArxContainer](https://github.com/hideakitai/ArxContainer)

## Build

To build this project, you need a Linux platform and a cross-compiling toolchain.

Currently tested on Ubuntu 20.04, with `gcc-riscv64-unknown-elf` and `qemu-system`.

After installing the necessary packages, clone the repo and initialize the submodules.

Then use CMake to configure and build. If everything goes well, you should get the kernel ELF `AbydOS_KNL` and its binary.

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

It's not necessary to build OpenSBI for a full installation of QEMU (-bios=default). if you don't, please compile it with `PLATFORM=generic` then run the QEMU using the command in `1.sh` under directory `opensbi`.  

## License

Under BSD-3.