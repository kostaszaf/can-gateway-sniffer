/******************************************************************************************
Copyright 2020 Konstantinos Zafeiropoulos. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
