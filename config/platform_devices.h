/**
 ****************************************************************************************
 *
 * @file platform_devices.h
 *
 * @brief Configuration of devices connected to board
 *
 * Copyright (C) 2017-2020 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef PLATFORM_DEVICES_H_
#define PLATFORM_DEVICES_H_

#include "peripheral_setup.h"
#include "ad_spi.h"
#include "ad_uart.h"
#include "hw_spi.h"

#if dg_configSPI_ADAPTER || dg_configUSE_SNC_HW_SPI
/*
 * SPI DEVICES
 *****************************************************************************************
 */
/**
 * \brief SPI device handle
 */
typedef const void* spi_device;


extern spi_device const MSC2518FD;

extern spi_device const MSC2518FD2;

#endif /* dg_configSPI_ADAPTER || dg_configUSE_SNC_HW_SPI */

#endif /* PLATFORM_DEVICES_H_ */
