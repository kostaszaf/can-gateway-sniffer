/**
 ****************************************************************************************
 *
 * @file platform_devices.c
 *
 * @brief Configuration of devices connected to board data structures
 *
 * Copyright (C) 2017-2020 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include "platform_devices.h"

/*
 * PLATFORM PERIPHERALS GPIO CONFIGURATION
 *****************************************************************************************
 */

#if dg_configSPI_ADAPTER || dg_configUSE_SNC_HW_SPI

static const ad_io_conf_t io_cs_SPI1[] = {
        {
                .port = MSC2518FD_SPI_CS_PORT, .pin = MSC2518FD_SPI_CS_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_SPI_EN,  true  },
                .off = { HW_GPIO_MODE_OUTPUT,           HW_GPIO_FUNC_GPIO ,   true  }
        },
};

static const ad_io_conf_t io_cs_SPI2[] = {
        {
                .port = MSC2518FD_SPI2_CS_PORT, .pin = MSC2518FD_SPI2_CS_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_SPI2_EN,  true  },
                .off = { HW_GPIO_MODE_OUTPUT,           HW_GPIO_FUNC_GPIO ,   true  }
        },
};

/* SPI1 I/O configuration */
const ad_spi_io_conf_t io_SPI1 = {
        .spi_do = {
                .port = SPI1_DO_GPIO_PORT, .pin = SPI1_DO_GPIO_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_SPI_DO,  false },
                .off = { HW_GPIO_MODE_OUTPUT,           HW_GPIO_FUNC_GPIO,    true  },
        },
        .spi_di = {
                .port = SPI1_DI_GPIO_PORT, .pin = SPI1_DI_GPIO_PIN,
                .on =  { HW_GPIO_MODE_INPUT_PULLDOWN,   HW_GPIO_FUNC_SPI_DI,  false },
                .off = { HW_GPIO_MODE_INPUT,            HW_GPIO_FUNC_GPIO,    true  },
        },
        .spi_clk = {
                .port = SPI1_CLK_GPIO_PORT, .pin = SPI1_CLK_GPIO_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_SPI_CLK, false },
                .off = { HW_GPIO_MODE_OUTPUT,           HW_GPIO_FUNC_GPIO,    true  },
        },
        .cs_cnt = sizeof(io_cs_SPI1) / sizeof(io_cs_SPI1[0]),
        .spi_cs = io_cs_SPI1,
        .voltage_level = HW_GPIO_POWER_V33
};

const ad_spi_io_conf_t io_SPI2 = {
        .spi_do = {
                .port = SPI2_DO_GPIO_PORT, .pin = SPI2_DO_GPIO_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_SPI2_DO,  false },
                .off = { HW_GPIO_MODE_OUTPUT,           HW_GPIO_FUNC_GPIO,    true  },
        },
        .spi_di = {
                .port = SPI2_DI_GPIO_PORT, .pin = SPI2_DI_GPIO_PIN,
                .on =  { HW_GPIO_MODE_INPUT_PULLDOWN,   HW_GPIO_FUNC_SPI2_DI,  false },
                .off = { HW_GPIO_MODE_INPUT,            HW_GPIO_FUNC_GPIO,    true  },
        },
        .spi_clk = {
                .port = SPI2_CLK_GPIO_PORT, .pin = SPI2_CLK_GPIO_PIN,
                .on =  { HW_GPIO_MODE_OUTPUT_PUSH_PULL, HW_GPIO_FUNC_SPI2_CLK, false },
                .off = { HW_GPIO_MODE_OUTPUT,           HW_GPIO_FUNC_GPIO,    true  },
        },
        .cs_cnt = sizeof(io_cs_SPI2) / sizeof(io_cs_SPI2[0]),
        .spi_cs = io_cs_SPI2,
        .voltage_level = HW_GPIO_POWER_V33
};


#endif /* dg_configSPI_ADAPTER || dg_configUSE_SNC_HW_SPI */


/*
 * PLATFORM PERIPHERALS CONTROLLER CONFIGURATION
 *****************************************************************************************
 */

#if dg_configSPI_ADAPTER || dg_configUSE_SNC_HW_SPI

/* MSC2518FD SPI driver configuration */
const ad_spi_driver_conf_t drv_MSC2518FD = {
        .spi.cs_pad             = { MSC2518FD_SPI_CS_PORT, MSC2518FD_SPI_CS_PIN },
        .spi.word_mode          = HW_SPI_WORD_8BIT,
        .spi.smn_role           = HW_SPI_MODE_MASTER,
     //   .spi.cpol_cpha_mode     = HW_SPI_CP_MODE_0,
        .spi.mint_mode          = HW_SPI_MINT_ENABLE,
        .spi.xtal_freq          = 1, //8 MHZ
        .spi.fifo_mode          = HW_SPI_FIFO_RX_TX,
        .spi.disabled           = 0,
     //   .spi.spi_cs = HW_SPI_CS_0,
		// TODO: TO be considered when LLD is updated to 32 FIFO size
     //   .spi.rx_tl = 5,
     //   .spi.tx_tl = HW_SPI_FIFO_LEVEL0,
     //   .spi.swap_bytes = false,
     //   .spi.select_divn = true,
#if (dg_configUSE_HW_DMA == 1)
        .spi.use_dma            = true,
        .spi.rx_dma_channel     = HW_DMA_CHANNEL_0,
        .spi.tx_dma_channel     = HW_DMA_CHANNEL_1
#endif /* dg_configUSE_HW_DMA */
};

const ad_spi_driver_conf_t drv_MSC2518FD2 = {
        .spi.cs_pad             = { MSC2518FD_SPI2_CS_PORT, MSC2518FD_SPI2_CS_PIN },
        .spi.word_mode          = HW_SPI_WORD_8BIT,
        .spi.smn_role           = HW_SPI_MODE_MASTER,
     //   .spi.cpol_cpha_mode     = HW_SPI_CP_MODE_0,
        .spi.mint_mode          = HW_SPI_MINT_ENABLE,
        .spi.xtal_freq          = 1, //8 MHZ
        .spi.fifo_mode          = HW_SPI_FIFO_RX_TX,
        .spi.disabled           = 0,
     //   .spi.spi_cs = HW_SPI_CS_0,
                // TODO: TO be considered when LLD is updated to 32 FIFO size
     //   .spi.rx_tl = 5,
     //   .spi.tx_tl = HW_SPI_FIFO_LEVEL0,
     //   .spi.swap_bytes = false,
     //   .spi.select_divn = true,
#if (dg_configUSE_HW_DMA == 1)
        .spi.use_dma            = true,
        .spi.rx_dma_channel     = HW_DMA_CHANNEL_2,
        .spi.tx_dma_channel     = HW_DMA_CHANNEL_3
#endif /* dg_configUSE_HW_DMA */
};

#endif /* dg_configSPI_ADAPTER || dg_configUSE_SNC_HW_SPI */


#if dg_configSPI_ADAPTER || dg_configUSE_SNC_HW_SPI

const ad_spi_controller_conf_t dev_MSC2518FD = {
        .id = HW_SPI1,
        .io = &io_SPI1,
        .drv = &drv_MSC2518FD
};
spi_device const MSC2518FD = &dev_MSC2518FD;

const ad_spi_controller_conf_t dev_MSC2518FD2 = {
        .id = HW_SPI2,
        .io = &io_SPI2,
        .drv = &drv_MSC2518FD2
};
spi_device const MSC2518FD2 = &dev_MSC2518FD2;

#endif /* dg_configSPI_ADAPTER */


