/*****************************************************************************
 *   A demo example using several of the peripherals on the base board
 *
 *   Copyright(C) 2011, EE2024
 *   All rights reserved.
 *
 ******************************************************************************/

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"

#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"

static uint8_t barPos = 2;

static char* msg = NULL;

volatile uint32_t msTicks; // counter for 1ms SysTicks

// ****************
//  SysTick_Handler - just increment SysTick counter
void SysTick_Handler(void) {
	msTicks++;
}

int getTicks(void) {
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

static void moveBar(uint8_t steps, uint8_t dir) {
	uint16_t ledOn = 0;

	if (barPos == 0)
		ledOn = (1 << 0) | (3 << 14);
	else if (barPos == 1)
		ledOn = (3 << 0) | (1 << 15);
	else
		ledOn = 0x07 << (barPos - 2);

	barPos += (dir * steps);
	barPos = (barPos % 16);

	pca9532_setLeds(ledOn, 0xffff);
}

//static void drawOled(uint8_t joyState) {
//	static int wait = 0;
//	static uint8_t currX = 48;
//	static uint8_t currY = 32;
//	static uint8_t lastX = 0;
//	static uint8_t lastY = 0;
//
//	if ((joyState & JOYSTICK_CENTER) != 0) {
//		oled_clearScreen(OLED_COLOR_BLACK);
//		return;
//	}
//
//	if (wait++ < 3)
//		return;
//
//	wait = 0;
//
//	if ((joyState & JOYSTICK_UP) != 0 && currY > 0) {
//		currY--;
//	}
//
//	if ((joyState & JOYSTICK_DOWN) != 0 && currY < OLED_DISPLAY_HEIGHT - 1) {
//		currY++;
//	}
//
//	if ((joyState & JOYSTICK_RIGHT) != 0 && currX < OLED_DISPLAY_WIDTH - 1) {
//		currX++;
//	}
//
//	if ((joyState & JOYSTICK_LEFT) != 0 && currX > 0) {
//		currX--;
//	}
//
//	if (lastX != currX || lastY != currY) {
//		oled_putPixel(currX, currY, OLED_COLOR_WHITE);
//		lastX = currX;
//		lastY = currY;
//	}
//}

#define NOTE_PIN_HIGH() GPIO_SetValue(0, 1<<26);
#define NOTE_PIN_LOW()  GPIO_ClearValue(0, 1<<26);

static uint32_t notes[] = { 2272, // A - 440 Hz
		2024, // B - 494 Hz
		3816, // C - 262 Hz
		3401, // D - 294 Hz
		3030, // E - 330 Hz
		2865, // F - 349 Hz
		2551, // G - 392 Hz
		1136, // a - 880 Hz
		1012, // b - 988 Hz
		1912, // c - 523 Hz
		1703, // d - 587 Hz
		1517, // e - 659 Hz
		1432, // f - 698 Hz
		1275, // g - 784 Hz
		};

static void playNote(uint32_t note, uint32_t durationMs) {

	uint32_t t = 0;

	if (note > 0) {

		while (t < (durationMs * 1000)) {
			NOTE_PIN_HIGH()
			;
			Timer0_us_Wait(note / 2);
			//delay32Us(0, note / 2);

			NOTE_PIN_LOW()
			;
			Timer0_us_Wait(note / 2);
			//delay32Us(0, note / 2);

			t += note;
		}

	} else {
		Timer0_Wait(durationMs);
		//delay32Ms(0, durationMs);
	}
}

static uint32_t getNote(uint8_t ch) {
	if (ch >= 'A' && ch <= 'G')
		return notes[ch - 'A'];

	if (ch >= 'a' && ch <= 'g')
		return notes[ch - 'a' + 7];

	return 0;
}

static uint32_t getDuration(uint8_t ch) {
	if (ch < '0' || ch > '9')
		return 400;

	/* number of ms */

	return (ch - '0') * 200;
}

static uint32_t getPause(uint8_t ch) {
	switch (ch) {
	case '+':
		return 0;
	case ',':
		return 5;
	case '.':
		return 20;
	case '_':
		return 30;
	default:
		return 5;
	}
}

static void playSong(uint8_t *song) {
	uint32_t note = 0;
	uint32_t dur = 0;
	uint32_t pause = 0;

	/*
	 * A song is a collection of tones where each tone is
	 * a note, duration and pause, e.g.
	 *
	 * "E2,F4,"
	 */

	while (*song != '\0') {
		note = getNote(*song++);
		if (*song == '\0')
			break;
		dur = getDuration(*song++);
		if (*song == '\0')
			break;
		pause = getPause(*song++);

		playNote(note, dur);
		//delay32Ms(0, pause);
		Timer0_Wait(pause);

	}
}

static uint8_t * song = (uint8_t*) "C2.C2,D4,C4,F4,E8,";
//(uint8_t*)"C2.C2,D4,C4,F4,E8,C2.C2,D4,C4,G4,F8,C2.C2,c4,A4,F4,E4,D4,A2.A2,H4,F4,G4,F8,";
//"D4,B4,B4,A4,A4,G4,E4,D4.D2,E4,E4,A4,F4,D8.D4,d4,d4,c4,c4,B4,G4,E4.E2,F4,F4,A4,A4,G8,";

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

	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 1;
	PinCfg.Pinnum = 31;
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
	// Initialize button
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

int main(void) {

	int monitor_state = 0;
	int current_tick = 48;
	int print_at_f_counter1 = 0;
	int print_at_f_counter2 = 0;
	int print_at_f_counter3 = 0;
	int movement = 0;

	int firedetected = 0;
	int movingindark = 0;
	int alertstatus;

	uint32_t previousTime;
	uint8_t data = 0;
	uint32_t len = 0;
	uint8_t line[64];

	init_uart();
	//test sending message
	msg = "POWER ON.\r\n";
	UART_Send(LPC_UART3, (uint8_t *) msg, strlen(msg), BLOCKING);

	int32_t xoff = 0;
	int32_t yoff = 0;
	int32_t zoff = 0;
	int8_t x = 0;
	int8_t y = 0;
	int8_t z = 0;
	int8_t xold = 0;
	int8_t yold = 0;
	int8_t zold = 64;

	uint8_t dir = 1;
	uint8_t wait = 0;

	uint8_t state = 0;

	uint8_t btn1 = 1;

	init_i2c();
	light_init();
	light_enable();
	init_ssp();
	init_GPIO();

	pca9532_init();
	joystick_init();
	acc_init();
	oled_init();
	led7seg_init();
	temp_init(*getTicks);
	rgb_init();

	/*
	 * Assume base board in zero-g position when reading first value.
	 */
	acc_read(&x, &y, &z);
	xoff = 0 - x;
	yoff = 0 - y;
	zoff = 64 - z;

	/* ---- Speaker ------> */

	GPIO_SetDir(2, 1 << 0, 1);
	GPIO_SetDir(2, 1 << 1, 1);

	GPIO_SetDir(0, 1 << 27, 1);
	GPIO_SetDir(0, 1 << 28, 1);
	GPIO_SetDir(2, 1 << 13, 1);
	GPIO_SetDir(0, 1 << 26, 1);

	GPIO_ClearValue(0, 1 << 27); //LM4811-clk
	GPIO_ClearValue(0, 1 << 28); //LM4811-up/dn
	GPIO_ClearValue(2, 1 << 13); //LM4811-shutdn

	/* <---- Speaker ------ */

	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1)
			;  // Capture error
	}

	moveBar(1, dir);

	GPIO_SetDir(2, 1 << 1, 1);

	GPIO_SetDir(1, 1 << 31, 0); //SW4

	/* <---- Initialize ------ */
	monitor_state = 0;
	rgb_setLeds(0);
	led7seg_setChar(32, FALSE);
	oled_clearScreen(OLED_COLOR_BLACK);

	while (1) {
		/* <---- State Manager ------ */
		if (((GPIO_ReadValue(1) >> 31) & 0x01) == 0 && monitor_state == 0) {
			//test sending message
			msg = "Entering MONITOR Mode.\r\n";
			UART_Send(LPC_UART3, (uint8_t *) msg, strlen(msg), BLOCKING);
			monitor_state = 1;
			rgb_setLeds(4);
			oled_clearScreen(OLED_COLOR_BLACK);
			oled_putString(10, 10, "MONITOR", OLED_COLOR_WHITE,
					OLED_COLOR_BLACK);
		} else if (((GPIO_ReadValue(1) >> 31) & 0x01) == 0
				&& monitor_state == 1) {
			//test sending message
			msg = "Entering Stable State.\r\n";
			UART_Send(LPC_UART3, (uint8_t *) msg, strlen(msg), BLOCKING);
			monitor_state = 0;
			rgb_setLeds(0);
			led7seg_setChar(32, FALSE);
			oled_clearScreen(OLED_COLOR_BLACK);
			systick_delay(1500);
		}

		/* <---- Monitor Mode ------ */
		if (monitor_state == 1) {

			// Run once every second
			while ((msTicks - previousTime) < 1000)
				;
			previousTime = msTicks;	// read current tick counter

			/* <---- Onboard timer shown on led7seg ------ */
			led7seg_setChar(current_tick, FALSE);
			current_tick++;
			if (current_tick == 58) {
				current_tick = 65;
			} else if (current_tick == 71) {
				current_tick = 48;
			}

			/* <---- Update sensors values ------ */
			// Read sensors (light, temp, acc)
			int lightval = light_read() + 48;
			int tempval = temp_read();
			acc_read(&x, &y, &z);
			// Adjust x y z readings
			x = x + xoff;
			y = y + yoff;
			z = z + zoff;
			// Check for movement based on previous acc values
			if (((abs(x - xold)) > 10) || ((abs(y - yold)) > 10)
					|| ((abs(z - zold)) > 10)) {
				movement = 1;
			} else {
				movement = 0;
			}
			xold = x;
			yold = y;
			zold = z;

			/* <---- LIGHT_LOW_WARNING & TEMP_HIGH_WARNING ------ */
			alertstatus = 1;
			// Check for user moving in dark places
			if (movement == 1 && lightval < 50) {
				movingindark = 1;
				alertstatus = alertstatus + 1;
			} else {
				movingindark = 0;
			}
			// Check if temp exceeds permissible limits
			if (tempval > 450) {
				firedetected = 1;
				alertstatus = alertstatus + 2;
			} else {
				firedetected = 0;
			}
			// Blink LED to alert user accordingly
			int blink_count = 0;
			switch (alertstatus) {
//			case 1: {
//				rgb_setLeds(0);
//			}
			case 2:
				while (blink_count < 2) { // Blink 3 times
					rgb_setLeds(2); // Set LED to BLUE
					systick_delay(100);
					rgb_setLeds(0); // Turn off LEDs
					systick_delay(100);
					blink_count++;
				}
			case 3:
				while (blink_count < 2) { // Blink 3 times
					rgb_setLeds(1); // Set LED to RED
					systick_delay(100);
					rgb_setLeds(0); // Turn off LEDs
					systick_delay(100);
					blink_count++;
				}
			case 4:
				while (blink_count < 2) { // Blink 3 times
					rgb_setLeds(1); // Set LED to RED
					systick_delay(100);
					rgb_setLeds(2); // Set LED to BLUE
					systick_delay(100);
					rgb_setLeds(0); // Turn off LEDs
					blink_count++;
				}
			}

//			if (y < 0) {
//				dir = 1;
//				y = -y;
//			} else {
//				dir = -1;
//			}
//
//			if (y > 1 && wait++ > (40 / (1 + (y / 10)))) {
//				moveBar(1, dir);
//				wait = 0;
//		}

			/* <---- Update values on OLED ------ */
			if (current_tick == 53 || current_tick == 65
					|| current_tick == 70) {
				char strLight[25], strTemp[25], strAcc[25];
				sprintf(strLight, "Light: %d", lightval);
				sprintf(strTemp, "Temp: %d", tempval);
				sprintf(strAcc, "A: %d %d %d", x, y, z);

				oled_clearScreen(OLED_COLOR_BLACK);
				oled_putString(10, 10, "MONITOR", OLED_COLOR_WHITE,
						OLED_COLOR_BLACK);
				oled_putString(10, 22, strTemp, OLED_COLOR_WHITE,
						OLED_COLOR_BLACK);
				oled_putString(10, 34, strLight, OLED_COLOR_WHITE,
						OLED_COLOR_BLACK);
				oled_putString(10, 46, strAcc, OLED_COLOR_WHITE,
						OLED_COLOR_BLACK);
			}

			/* <---- Send message via UART ------ */
			if (current_tick == 70) {
				// Warning messages sent first
				if (firedetected == 1) {
					char redmsg[30] = "Fire was Detected.\r\n";
					UART_Send(LPC_UART3, (uint8_t *) redmsg, strlen(redmsg),
							BLOCKING);
				}
				if (movingindark == 1) {
					char bluemsg[40] = "Movement in darkness was Detected.\r\n";
					UART_Send(LPC_UART3, (uint8_t *) bluemsg, strlen(bluemsg),
							BLOCKING);
				}

				char strToSend[70];
				sprintf(strToSend, "%d%d%d_-_T%d_L%d_AX%d_AY%d_AZ%d\r\n",
						print_at_f_counter3, print_at_f_counter2,
						print_at_f_counter1, tempval, lightval, x, y, z);
				UART_Send(LPC_UART3, (uint8_t *) strToSend, strlen(strToSend),
						BLOCKING);

				print_at_f_counter1++;
				if (print_at_f_counter1 > 9) {
					print_at_f_counter1 = 0;
					print_at_f_counter2++;
					if (print_at_f_counter2 > 9) {
						print_at_f_counter3++;
					}
				}
			}

			/* ####### Joystick and OLED  ###### */
			/* # */

//			state = joystick_read();
//			if (state != 0)
//				drawOled(state);
			/* # */
			/* ############################################# */

			/*if (btn1 == 0)
			 {
			 playSong(song);
			 } */

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
