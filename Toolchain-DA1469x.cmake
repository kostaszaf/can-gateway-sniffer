cmake_minimum_required (VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# define the cross compiler
set(CROSS arm-none-eabi-)
set(CMAKE_C_COMPILER ${CROSS}gcc)
set(CMAKE_OBJCOPY ${CROSS}objcopy CACHE STRING "The objcopy utility of the cross toolchain" FORCE)
set(CMAKE_NM ${CROSS}gcc-nm CACHE STRING "The nm utility of the cross toolchain" FORCE)
set(CMAKE_AR ${CROSS}gcc-ar CACHE STRING "The ar utility of the cross toolchain" FORCE)
set(CMAKE_SIZE ${CROSS}size CACHE STRING "The size utility of the cross toolchain" FORCE)
# set(CMAKE_C_ARCHIVE_FINISH true)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#c flags macros

set (CMAKE_C_STANDARD 11)

# Flags for Link-Time-Optimization
set(OPT_LTO_FLAGS -flto -ffat-lto-objects)

# Flags to use when LTO is not used; LTO produces smaller images without them,
# but they are much better than nothing if LTO cannot be used
set(OPT_NON_LTO_FLAGS -ffunction-sections -fdata-sections)

set(FP_FLAGS -mfloat-abi=hard -mfpu=fpv5-sp-d16)

set(NON_FP_FLAGS -mfloat-abi=soft)

set(OTHER_CFLAGS "-mcpu=cortex-m33 -Wall -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Os -std=gnu11") 

#other tools, scripts etc. TBA 

