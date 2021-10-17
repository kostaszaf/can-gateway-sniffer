cmake_minimum_required (VERSION 3.20)

project (q3obdutility C ASM)

file(TO_CMAKE_PATH "${ROOT_PATH}" TOP_DIR)

set(SDK_DIR ${TOP_DIR}/${SDK_VERSION})

add_definitions(
        -Ddg_configDEVICE=DA14699_00
        -DRELEASE_BUILD
)

add_compile_options(-Os)

set(CMAKE_C_FLAGS "${OTHER_CFLAGS}" CACHE STRING "" FORCE)

# set link flags
set(CMAKE_EXE_LINKER_FLAGS "-Wall -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Os -Xlinker --gc-sections --specs=nano.specs --specs=nosys.specs" CACHE STRING "Linker flags for DA1469x-syscpu" FORCE)

# Extra compilation flags for M33 (SYSCPU)
set(M33_FLAGS ${OPT_NON_LTO_FLAGS} ${FP_FLAGS} -mcpu=cortex-m33  -mthumb)

set(SYSCPU_C_SRC_FILES
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_crypto.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_flash.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_gpadc.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_haptic.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_i2c.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_iso7816.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_keyboard_scanner.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_lcdc.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_nvms.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_nvms_direct.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_nvms_ves.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_nvparam.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_pmu.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_sdadc.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_snc.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_spi.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_template.c
        ${SDK_DIR}/sdk/middleware/adapters/src/ad_uart.c
        ${SDK_DIR}/sdk/middleware/adapters/src/sys_platform_devices_internal.c 
        ${SDK_DIR}/sdk/free_rtos/croutine.c
        ${SDK_DIR}/sdk/free_rtos/event_groups.c
        ${SDK_DIR}/sdk/free_rtos/list.c
        ${SDK_DIR}/sdk/free_rtos/queue.c
        ${SDK_DIR}/sdk/free_rtos/stream_buffer.c
        ${SDK_DIR}/sdk/free_rtos/tasks.c
        ${SDK_DIR}/sdk/free_rtos/timers.c 
        ${SDK_DIR}/sdk/free_rtos/portable/Common/mpu_wrappers.c 
        ${SDK_DIR}/sdk/free_rtos/portable/GCC/DA1469x/port.c 
        ${SDK_DIR}/sdk/free_rtos/portable/MemMang/heap_4.c 
        ${SDK_DIR}/sdk/bsp/memory/src/qspi_automode.c 
        ${SDK_DIR}/sdk/middleware/osal/msg_queues.c
        ${SDK_DIR}/sdk/middleware/osal/resmgmt.c
        ${SDK_DIR}/sdk/middleware/osal/usb_osal_wrapper.c 
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_aes_hash.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_apu_src.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_bod_da1469x.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_cache.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_charger_da1469x.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_clk_da1469x.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_cpm_da1469x.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_crypto.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_dma.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_gpadc.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_gpio.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_haptic.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_hard_fault.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_i2c.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_iso7816.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_lcdc.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_memctrl.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_otpc_da1469x.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_pcm.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_pdc.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_pdm.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_pmu_da1469x.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_qspi.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_rtc.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_sdadc.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_smotor.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_snc.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_spi.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_sys_da1469x.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_timer.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_trng.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_uart.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_usb_da1469x.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_watchdog.c
        ${SDK_DIR}/sdk/bsp/peripherals/src/hw_wkup_da1469x.c 
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_adc.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_audio_mgr.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_bsr.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_charger_da1469x.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_clock_mgr_da1469x.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_power_mgr_da1469x.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_rcx_calibrate.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_tcs_da1469x.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_timer.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_trng.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_usb_da1469x.c
        ${SDK_DIR}/sdk/bsp/system/sys_man/sys_watchdog.c 
        ${SDK_DIR}/sdk/bsp/util/src/sdk_crc16.c
        ${SDK_DIR}/sdk/bsp/util/src/sdk_list.c
        ${SDK_DIR}/sdk/bsp/util/src/sdk_queue.c 
        ${SDK_DIR}/sdk/bsp/startup/config.c 
        ${SDK_DIR}/sdk/bsp/startup/DA1469x/init_da1469x.c
        ${SDK_DIR}/sdk/bsp/startup/DA1469x/system_da1469x.c 
        ${SDK_DIR}/sdk/bsp/startup/DA1469x/GCC/deepsleep.S
        ${SDK_DIR}/sdk/bsp/startup/DA1469x/GCC/exception_handlers.S
        ${SDK_DIR}/sdk/bsp/startup/DA1469x/GCC/startup_da1469x.S
        ${SDK_DIR}/sdk/bsp/startup/DA1469x/GCC/vector_table_da1469x.S 
        ${ROOT_PATH}/config/ee_printf.c
        ${ROOT_PATH}/config/platform_devices.c 
        ${ROOT_PATH}/mcp25xxfd_driver/canfdspi/drv_canfdspi_api.c 
        ${ROOT_PATH}/application/main.c 
        ${ROOT_PATH}/application/app.c
        ${ROOT_PATH}/application/app_pid.c
)

set(SYSCPU_INCLUDE_DIRS
        ${SDK_DIR}/sdk/middleware/adapters/include
        ${ROOT_PATH}/mcp25xxfd_driver/canfdspi
        ${ROOT_PATH}/config
        ${SDK_DIR}/sdk/bsp/util/include
        ${SDK_DIR}/sdk/bsp/memory/include
        ${SDK_DIR}/sdk/bsp/config
        ${SDK_DIR}/sdk/middleware/config
        ${SDK_DIR}/sdk/bsp/include
        ${SDK_DIR}/sdk/free_rtos/include
        ${SDK_DIR}/sdk/free_rtos/portable/GCC/DA1469x
        ${SDK_DIR}/sdk/middleware/osal
        ${SDK_DIR}/sdk/bsp/system/sys_man/include
        ${SDK_DIR}/sdk/bsp/system/sys_man
        ${SDK_DIR}/sdk/bsp/peripherals/include
        ${SDK_DIR}/sdk/bsp/snc/include
        ${ROOT_PATH}/application
)

set(SYSCPU_EXEC ${PROJECT_NAME})
set(SYSCPU_ELF ${SYSCPU_EXEC}.elf)
set(SYSCPU_BIN ${SYSCPU_EXEC}.bin)
set(SYSCPU_HEX ${SYSCPU_EXEC}.hex)
set(SYSCPU_MAP ${SYSCPU_EXEC}.map)

set(CUSTOM_CONFIG  ${ROOT_PATH}/config/custom_config_qspi.h)

add_executable(${SYSCPU_ELF}
        ${CUSTOM_CONFIG}
        ${SYSCPU_C_SRC_FILES}
)

target_compile_definitions(${SYSCPU_ELF} PUBLIC -DCORTEX_M33)

target_compile_options(${SYSCPU_ELF} PUBLIC ${M33_FLAGS} -include ${CUSTOM_CONFIG})

target_include_directories(${SYSCPU_ELF} PUBLIC
        ${SYSCPU_INCLUDE_DIRS}
)

# Generate Linker script
set(SYSCPU_LDSCRIPT "sections_da1469x.ld")
set(MEM_LDSCRIPT "mem.ld")

add_custom_target(${SYSCPU_LDSCRIPT}
        COMMAND ${CMAKE_C_COMPILER} -I ${SDK_DIR}/sdk/bsp/config -I ${SDK_DIR}/sdk/middleware/config 
                -imacros ${ROOT_PATH}/config/custom_config_qspi.h -DRELEASE_BUILD -Ddg_configDEVICE=DA14699_00  
                -E -P -c ${ROOT_PATH}/config/${SYSCPU_LDSCRIPT}.h -o ${SYSCPU_LDSCRIPT}
        COMMAND ${CMAKE_C_COMPILER} -I ${SDK_DIR}/sdk/bsp/config -I ${SDK_DIR}/sdk/middleware/config 
                -imacros ${ROOT_PATH}/config/custom_config_qspi.h -DRELEASE_BUILD -Ddg_configDEVICE=DA14699_00  
                -E -P -c ${SDK_DIR}/sdk/bsp/ldscripts/ble_projects/${MEM_LDSCRIPT}.h -o ${MEM_LDSCRIPT}
        DEPENDS ${SYSCPU_LDSCRIPT} ${CUSTOM_CONFIG}
        COMMENT "Generating ${SYSCPU_LDSCRIPT} and ${MEM_LDSCRIPT}..."
        COMMAND_EXPAND_LISTS
)

set_target_properties(${SYSCPU_ELF} PROPERTIES LINK_DEPENDS ${SYSCPU_LDSCRIPT} )

target_link_libraries(${SYSCPU_ELF} ${SYS_CPU_BSP}
        ${M33_FLAGS}
        "-Wl,-Map,${SYSCPU_MAP}  -T${MEM_LDSCRIPT} -T${SYSCPU_LDSCRIPT}")

# create .bin
add_custom_target(${SYSCPU_BIN} ALL
        DEPENDS ${SYSCPU_ELF}
        COMMAND ${CMAKE_OBJCOPY} -Obinary ${SYSCPU_ELF} ${SYSCPU_BIN} -R .cmac_ram
        COMMENT "Create binary image (${SYSCPU_BIN})"
)

# create .hex
add_custom_command(TARGET ${SYSCPU_ELF} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex ${SYSCPU_ELF} ${SYSCPU_HEX}
        COMMENT "Create hex image (${SYSCPU_HEX})"
)

# Add target for installing SYSCPU executable
install(TARGETS ${SYSCPU_ELF}
        RUNTIME DESTINATION ${CMAKE_BINARY_DIR}
        COMPONENT SYSCPU
)

install(FILES   ${CMAKE_CURRENT_BINARY_DIR}/${SYSCPU_BIN}
                ${CMAKE_CURRENT_BINARY_DIR}/${SYSCPU_HEX}
                ${CMAKE_CURRENT_BINARY_DIR}/${SYSCPU_MAP}
        DESTINATION ${CMAKE_BINARY_DIR}
        COMPONENT SYSCPU
)


