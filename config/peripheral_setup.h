/**
 ****************************************************************************************
 *
 * @file peripheral_setup.h
 *
 * @brief Peripherals setup header file.
 *
 * Copyright (C) 2017-2020 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef PERIPHERAL_SETUP_H_
#define PERIPHERAL_SETUP_H_

#define MICROBUS_SLOT1
#define MICROBUS_SLOT2

#ifdef MICROBUS_SLOT1

#define SPI1_DO_GPIO_PORT               (HW_GPIO_PORT_0)
#define SPI1_DO_GPIO_PIN                (HW_GPIO_PIN_26)

#define SPI1_DI_GPIO_PORT               (HW_GPIO_PORT_0)
#define SPI1_DI_GPIO_PIN                (HW_GPIO_PIN_24)

#define SPI1_CLK_GPIO_PORT              (HW_GPIO_PORT_0)
#define SPI1_CLK_GPIO_PIN               (HW_GPIO_PIN_21)


#define MSC2518FD_SPI_CS_PORT             (HW_GPIO_PORT_0)
#define MSC2518FD_SPI_CS_PIN              (HW_GPIO_PIN_20)

#define INT_PORT                        (HW_GPIO_PORT_0)
#define INT_PIN                         (HW_GPIO_PIN_27)

#define INT0_PORT                       (HW_GPIO_PORT_0)
#define INT0_PIN                        (HW_GPIO_PIN_28)

#define INT1_PORT                       (HW_GPIO_PORT_0)
#define INT1_PIN                        (HW_GPIO_PIN_29)

#endif

#ifdef MICROBUS_SLOT2

#define SPI2_DO_GPIO_PORT               (HW_GPIO_PORT_1)
#define SPI2_DO_GPIO_PIN                (HW_GPIO_PIN_5)

#define SPI2_DI_GPIO_PORT               (HW_GPIO_PORT_1)
#define SPI2_DI_GPIO_PIN                (HW_GPIO_PIN_4)

#define SPI2_CLK_GPIO_PORT              (HW_GPIO_PORT_1)
#define SPI2_CLK_GPIO_PIN               (HW_GPIO_PIN_3)

#define MSC2518FD_SPI2_CS_PORT             (HW_GPIO_PORT_1)
#define MSC2518FD_SPI2_CS_PIN              (HW_GPIO_PIN_2)

#define INT2_PORT                        (HW_GPIO_PORT_1)
#define INT2_PIN                         (HW_GPIO_PIN_7)

#define INT20_PORT                       (HW_GPIO_PORT_1)
#define INT20_PIN                        (HW_GPIO_PIN_8)

#define INT21_PORT                       (HW_GPIO_PORT_0)
#define INT21_PIN                        (HW_GPIO_PIN_17)

#endif


#endif /* PERIPHERAL_SETUP_H_ */
