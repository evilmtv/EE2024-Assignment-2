/*****************************************************************************
 *	Assignment 2
 *	Lim Jun Hao	(A0147944U)
 *	Luo Ruikang	(A0164638W)
 *
 *	NOTE: Code is not refactorized for readability
 *
 *	Code based off EE2024 demo code
 *	Copyright(C) 2011, EE2024
 *	All rights reserved.
 *
 ******************************************************************************/

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"
#include "light.h"
#include "temp.h"

int32_t refreshRate = 1000;
int8_t alertStatus = 0;
static char* msg = NULL;
volatile uint32_t msTicks; // counter for 1ms SysTicks

//  SysTick_Handler - increment SysTick counter
void SysTick_Handler(void) {
	msTicks++;

	// Blink LED to alert user accordingly
	if (alertStatus == 1) {
		if (msTicks % 333 == 0) {
			rgb_setLeds(2); // Set LED to BLUE
		}
		if ((msTicks + 167) % 333 == 0) {
			rgb_setLeds(0); // Turn off LEDs
		}
	} else if (alertStatus == 2) {
		if (msTicks % 333 == 0) {
			rgb_setLeds(1); // Set LED to RED
		}
		if ((msTicks + 167) % 333 == 0) {
			rgb_setLeds(0); // Turn off LEDs
		}
	} else if (alertStatus == 3) {
		if (msTicks % 333 == 0) {
			rgb_setLeds(2); // Set LED to BLUE
		}
		if ((msTicks + 167) % 333 == 0) {
			rgb_setLeds(1); // Set LED to RED
		}
	}
}

uint32_t getTicks(void) {
	return msTicks;
}

// ****************
// systick_delay - creates a delay of the appropriate number of Systicks (happens every 1 ms)
__INLINE void systick_delay(uint32_t delayTicks) {
	uint32_t currentTicks;

	currentTicks = msTicks;	// read current tick counter
	// Now loop until required number of ticks passes.
	while ((msTicks - currentTicks) < delayTicks)
		;
}

static void init_ssp(void) {
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;
	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);
	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);
}

static void init_i2c(void) {
	PINSEL_CFG_Type PinCfg;
	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);
	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);
	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_GPIO(void) {
	PINSEL_CFG_Type PinCfg;
	// Initialize button SW4
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 1;
	PinCfg.Pinnum = 31;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, 1 << 31, 0);

	//Initialize button SW3
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 10;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(2, 1 << 10, 0);
}

void pinsel_uart3(void) {
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 0;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);
}

void init_uart(void) {
	UART_CFG_Type uartCfg;
	uartCfg.Baud_rate = 115200;
	uartCfg.Databits = UART_DATABIT_8;
	uartCfg.Parity = UART_PARITY_NONE;
	uartCfg.Stopbits = UART_STOPBIT_1;
	//pin select for uart3;
	pinsel_uart3();
	//supply power & setup working parameters for uart3
	UART_Init(LPC_UART3, &uartCfg);
	//enable transmit for uart3
	UART_TxCmd(LPC_UART3, ENABLE);
}

void initialize() {
	init_uart();
	init_i2c();
	light_init();
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	init_ssp();
	init_GPIO();
	pca9532_init();
	joystick_init();
	acc_init();
	oled_init();
	led7seg_init();
	temp_init(*getTicks);
	rgb_init();

	// Enable GPIO Interrupt P0.4 for SW3 (reset button)
	LPC_GPIOINT ->IO0IntEnF |= 1 << 4;
	NVIC_EnableIRQ(EINT3_IRQn);
	// set up interrupt priority
	NVIC_SetPriorityGrouping(5);
	NVIC_SetPriority(EINT3_IRQn, (1 << (__NVIC_PRIO_BITS - 1))); // 10|000|000, first priority for external interrupt
}

void EINT3_IRQHandler(void) {
	// SW3
	if ((LPC_GPIOINT ->IO0IntStatF >> 4) & 0x1) {
		LPC_GPIOINT ->IO0IntClr |= 1 << 4;
		if (refreshRate == 1000) {
			refreshRate = 1;
		} else if (refreshRate == 1) {
			refreshRate = 2000;
		} else {
			refreshRate = 1000;
		}
	}
}

int main(void) {

	/*
	 * Singed (int), unsigned (uint), bits [range=2^x]
	 * 8bit unsigned -> 0~255
	 * 8bit signed -> -128~+127
	 * "A good example is long. On one machine, it might be 32 bits (the minimum required by C). On another, it's 64 bits."
	 */
	uint32_t debounceTime = 0;
	uint8_t monitorState = 0;
	uint32_t currentTick = 48;
	uint8_t uartCounterThirdDigit = 0;
	uint8_t uartCounterSecondDigit = 0;
	uint8_t uartCounterFirstDigit = 0;
	uint8_t movementDetected = 0;
	uint8_t fireDetected = 0;
	uint8_t movingInDarkDetected = 0;
	uint32_t previousTime;

	// acc can measure up to ±70 (±64 typ)
	int8_t xoff = 0;
	int8_t yoff = 0;
	int8_t zoff = 0;
	int8_t x = 0;
	int8_t y = 0;
	int8_t z = 0;
	int8_t xold = 0;
	int8_t yold = 0;
	int8_t zold = 64;

	initialize();

	/* ---- Calibrate ------> */

	acc_read(&x, &y, &z);
	xoff = 0 - x;
	yoff = 0 - y;
	zoff = 64 - z;

	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1)
			;  // Capture error
	}

	/* <---- Initialize ------ */
	//test sending message
	msg = "POWER ON.\r\n";
	UART_Send(LPC_UART3, (uint8_t *) msg, strlen(msg), BLOCKING);
	monitorState = 0;
	rgb_setLeds(0);
	led7seg_setChar(32, FALSE);
	oled_clearScreen(OLED_COLOR_BLACK);
	previousTime = msTicks;

	while (1) {
		/* <---- State Manager ------ */
		if (((GPIO_ReadValue(1) >> 31) & 0x01) == 0) {
			if ((msTicks - debounceTime) > 500) {
				if (monitorState == 0) {
					msg = "Entering MONITOR Mode.\r\n";
					UART_Send(LPC_UART3, (uint8_t *) msg, strlen(msg),
							BLOCKING);
					monitorState = 1;
					rgb_setLeds(4); // Turn on Green LED on rgbLED to enable oled screen
					oled_clearScreen(OLED_COLOR_BLACK);
					oled_putString(10, 10, "MONITOR", OLED_COLOR_WHITE,
							OLED_COLOR_BLACK);
				} else if (monitorState == 1) {
					msg = "Entering Stable State.\r\n";
					UART_Send(LPC_UART3, (uint8_t *) msg, strlen(msg),
							BLOCKING);
					monitorState = 0; // Stable state
					rgb_setLeds(0); // Turn off rgbLeds
					led7seg_setChar(32, FALSE); // Clear led7seg display
					oled_clearScreen(OLED_COLOR_BLACK); // Clear oled screen
					fireDetected = 0; // Reset fireDetected flag
					movingInDarkDetected = 0; // Reset movingInDarkDetected flag
					alertStatus = 0; // Reset alert flag
					refreshRate = 1000; // Reset refresh rate
				}
			}
			debounceTime = msTicks;
		}

		/* <---- Monitor Mode ------ */
		if (monitorState == 1) {

			/* <---- refreshRateHandler ------ */
			// Run once every second by default
			while ((msTicks - previousTime) < refreshRate) {
				; // Wait
			}
			previousTime = msTicks;	// read current tick counter

			/* <---- updateLed7SegTick(a) ------ */
			led7seg_setChar(currentTick, FALSE);

			/* <---- readAndProcessSensorValues ------ */
			// Read sensors (light, temp, acc)
			int lightval = light_read() + 48;
			int tempval = temp_read();
			acc_read(&x, &y, &z);
			// Adjust x y z readings to easier to read format based on earlier calibrations
			x = x + xoff;
			y = y + yoff;
			z = z + zoff;
			if (movingInDarkDetected == 0) { // Reduce computation if movingInDarkDetected is already true
				// Check for movement based on previous acc values
				if (((abs(x - xold)) > 10) || ((abs(y - yold)) > 10)
						|| ((abs(z - zold)) > 10)) {
					movementDetected = 1;
				} else {
					movementDetected = 0;
				}
				// Update 'previous' acc values to current values
				xold = x;
				yold = y;
				zold = z;
			}

			/* <---- checkProcessedSensorValuesForDanger ------ */
			if (alertStatus != 3) { // Reduce computation if unnecessary
				// Check for user moving in dark places
				if (movingInDarkDetected == 0 && movementDetected == 1
						&& lightval < 50) {
					movingInDarkDetected = 1;
					if (fireDetected == 1) {
						alertStatus = 3;
					} else {
						alertStatus = 1;
					}
				}
				// Check if temp exceeds permissible limits
				if (fireDetected == 0 && tempval > 450) {
					fireDetected = 1;
					if (movingInDarkDetected == 1) {
						alertStatus = 3;
					} else {
						alertStatus = 2;
					}
				}
			}

			/* <---- printProcessedSensorValuesOnOled ------ */
			if (currentTick == 53 || currentTick == 65 || currentTick == 70) {
				oled_clearScreen(OLED_COLOR_BLACK);
				char strLight[25], strTemp[25], strAcc[25], strRefresh[25];
				sprintf(strLight, "Light: %d", lightval);
				sprintf(strTemp, "Temp: %d", tempval);
				sprintf(strAcc, "A: %d %d %d", x, y, z);
				if (refreshRate == 1) {
					sprintf(strRefresh, "BOOST");
					oled_putString(45, 10, strRefresh, OLED_COLOR_WHITE,
							OLED_COLOR_BLACK);
				} else if (refreshRate == 2000) {
					sprintf(strRefresh, "ECO");
					oled_putString(45, 10, strRefresh, OLED_COLOR_WHITE,
							OLED_COLOR_BLACK);
				}
				oled_putString(1, 10, "MONITOR", OLED_COLOR_WHITE,
						OLED_COLOR_BLACK);
				oled_putString(1, 22, strTemp, OLED_COLOR_WHITE,
						OLED_COLOR_BLACK);
				oled_putString(1, 34, strLight, OLED_COLOR_WHITE,
						OLED_COLOR_BLACK);
				oled_putString(1, 46, strAcc, OLED_COLOR_WHITE,
						OLED_COLOR_BLACK);
			}

			/* <---- sendUpdateToUart ------ */
			if (currentTick == 70) {
				// Warning messages sent first
				if (fireDetected == 1) {
					char redmsg[30] = "Fire was Detected.\r\n";
					UART_Send(LPC_UART3, (uint8_t *) redmsg, strlen(redmsg),
							BLOCKING);
				}
				if (movingInDarkDetected == 1) {
					char bluemsg[40] = "Movement in darkness was Detected.\r\n";
					UART_Send(LPC_UART3, (uint8_t *) bluemsg, strlen(bluemsg),
							BLOCKING);
				}

				char strToSend[70];
				sprintf(strToSend, "%d%d%d_-_T%d_L%d_AX%d_AY%d_AZ%d\r\n",
						uartCounterFirstDigit, uartCounterSecondDigit,
						uartCounterThirdDigit, tempval, lightval, x, y, z);
				UART_Send(LPC_UART3, (uint8_t *) strToSend, strlen(strToSend),
						BLOCKING);

				uartCounterThirdDigit++;
				if (uartCounterThirdDigit > 9) {
					uartCounterThirdDigit = 0;
					uartCounterSecondDigit++;
					if (uartCounterSecondDigit > 9) {
						uartCounterSecondDigit = 0;
						uartCounterFirstDigit++;
					}
				}
			}

			/* <---- updateLed7SegTick(b) ------ */
			currentTick++;
			if (currentTick == 58) {
				currentTick = 65;
			} else if (currentTick == 71) {
				currentTick = 48;
			}

			Timer0_Wait(1);
		}
	}
}

void check_failed(uint8_t *file, uint32_t line) {
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
		;
}
