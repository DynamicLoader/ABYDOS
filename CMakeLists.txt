cmake_minimum_required(VERSION 3.0.0)
project(AbydOS_Kernel VERSION 0.1.0 LANGUAGES C CXX ASM)

# -DCMAKE_C_COMPILER_WORKS:STRING=1 -DCMAKE_CXX_COMPILER_WORKS:STRING=1 should be passed into cmake since unknown-elf toolchain does not compile normal program

# set(CMAKE_SYSTEM_NAME "Generic")
# set(CMAKE_SYSTEM "Generic")
set(CMAKE_SYSTEM_PROCESSOR rv64)

set(Toolchain_Prefix riscv64-unknown-elf-)

set(PL_ABI lp64d)
set(PL_ISA rv64imafdc)
set(PL_MDL medany)

# get rid of '-rdynamic'
set(CMAKE_STATIC_LINKER_FLAGS " ")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# set(KernelStackSize 0x10000) # also change in test.elf.ldS

set(CMAKE_C_FLAGS "-g -Wall -O0 -Werror -ffunction-sections -fdata-sections -ffreestanding -specs=nosys.specs -nostartfiles -fno-stack-protector -fno-strict-aliasing -fno-omit-frame-pointer -fno-optimize-sibling-calls -fPIE")
set(CMAKE_CXX_FLAGS "-g -Wall -O0 -Werror -ffunction-sections -fdata-sections  -specs=nosys.specs -nostartfiles -fno-stack-protector -fno-strict-aliasing -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-rtti -fPIE")

set(CMAKE_EXE_LINKER_FLAGS " ")
set(CMAKE_ASM_FLAGS "-g -Wall -nostdlib -fno-omit-frame-pointer -fno-optimize-sibling-calls -fPIE")

file(GLOB KNL_SRC 
    # kernel
    ${CMAKE_SOURCE_DIR}/kernel/main/*.c
    ${CMAKE_SOURCE_DIR}/kernel/main/*.cpp
    ${CMAKE_SOURCE_DIR}/kernel/main/*.S

    # builtin drivers
    ${CMAKE_SOURCE_DIR}/kernel/driver/*.c
    ${CMAKE_SOURCE_DIR}/kernel/driver/*.cpp
    ${CMAKE_SOURCE_DIR}/kernel/driver/*/*.c
    ${CMAKE_SOURCE_DIR}/kernel/driver/*/*.cpp

    # libfdt
    ${CMAKE_SOURCE_DIR}/deps/libfdt/*.c
)

add_executable(AbydOS_KNL ${KNL_SRC})

target_include_directories(AbydOS_KNL 
    PUBLIC ${CMAKE_SOURCE_DIR}/opensbi/include
    PUBLIC ${CMAKE_SOURCE_DIR}/kernel/inc
    PUBLIC ${CMAKE_SOURCE_DIR}/deps/libfdt/
)

target_compile_options(AbydOS_KNL 
    PUBLIC -mabi=${PL_ABI}
    PUBLIC -march=${PL_ISA}
    PUBLIC -mcmodel=${PL_MDL}
    # PUBLIC -DDEBUG
)

target_link_options(AbydOS_KNL
    
    # PUBLIC  -Wl,--no-relax
    PUBLIC -Wl,--build-id=none
    # PUBLIC -fno-pie -no-pie
    # PUBLIC -flinker-output=pie
    PUBLIC -static-pie
    PUBLIC -T ${CMAKE_SOURCE_DIR}/kernel/main/kernel.ldS
)

# target_link_libraries(AbydOS_KNL
#     PUBLIC -lsupc++
# )

add_custom_target(AbydOS_KNL_BIN ALL
    COMMAND ${Toolchain_Prefix}objcopy -S -O binary AbydOS_KNL AbydOS.bin
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS AbydOS.bin
    DEPENDS AbydOS_KNL
)