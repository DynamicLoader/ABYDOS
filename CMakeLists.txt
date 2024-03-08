cmake_minimum_required(VERSION 3.0.0)
project(AbydOS_Kernel VERSION 0.1.0 LANGUAGES C CXX ASM)

# -DCMAKE_C_COMPILER_WORKS:STRING=1 -DCMAKE_CXX_COMPILER_WORKS:STRING=1 should be passed into cmake since unknown-elf toolchain does not compile normal program

# set(CMAKE_SYSTEM_NAME "Generic")
# set(CMAKE_SYSTEM "Generic")
set(CMAKE_SYSTEM_PROCESSOR rv64)

set(Toolchain_Prefix riscv64-unknown-elf-)

set(PL_ABI lp64)
set(PL_ISA rv64imafdc)
set(PL_MDL medany)

# get rid of '-rdynamic'
set(CMAKE_STATIC_LINKER_FLAGS " ")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

set(CMAKE_C_FLAGS "-g -Wall -Werror -ffreestanding -nostdlib -fno-stack-protector -fno-strict-aliasing -fno-omit-frame-pointer -fno-optimize-sibling-calls -fPIC")
set(CMAKE_CXX_FLAGS "-g -Wall -Werror -ffreestanding -nostdlib -fno-stack-protector -fno-strict-aliasing -fno-omit-frame-pointer -fno-optimize-sibling-calls  -fno-exceptions -fPIC")

set(CMAKE_EXE_LINKER_FLAGS " ")
set(CMAKE_ASM_FLAGS "-g -Wall -nostdlib -fno-omit-frame-pointer -fno-optimize-sibling-calls -fpic")

file(GLOB KNL_SRC 
    ${CMAKE_SOURCE_DIR}/kernel/main/*.c
    ${CMAKE_SOURCE_DIR}/kernel/main/*.cpp
    ${CMAKE_SOURCE_DIR}/kernel/main/*.S
)

add_executable(AbydOS_KNL ${KNL_SRC})

target_include_directories(AbydOS_KNL 
    PUBLIC ${CMAKE_SOURCE_DIR}/opensbi/include
)

target_compile_options(AbydOS_KNL 
    PUBLIC -mabi=${PL_ABI}
    PUBLIC -march=${PL_ISA}
    PUBLIC -mcmodel=${PL_MDL}
)

target_link_options(AbydOS_KNL
    PUBLIC -T ${CMAKE_SOURCE_DIR}/kernel/main/test.elf.ldS
    PUBLIC -Wl,--build-id=none
    # PUBLIC -Wl,--exclude-libs,ALL
    PUBLIC -Wl,--no-dynamic-linker 
    # PUBLIC -fno-pie -no-pie # overrides C_FLAGS
)

add_custom_target(AbydOS_KNL_BIN ALL
    COMMAND ${Toolchain_Prefix}objcopy -S -O binary AbydOS_KNL AbydOS.bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS AbydOS.bin
    DEPENDS AbydOS_KNL
)