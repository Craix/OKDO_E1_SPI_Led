#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_debug_console.h"

//RGB LED Ring WS 28 12 B 5050 x 8 LEDs - 30mm

#define LEDS 8
#define COLOR 5

#define GET_BIT(k, n) (k & (1 << (n)))
#define SET_BIT(k, n) (k |= (1 << (n)))
#define CLR_BIT(k, n) (k &= ~(1 << (n)))

#define HRGB_to_GRB(c) ((c & (0xFF<<16))>>8) + ((c &(0xFF<<8))<<8) + (c & 0xFF)
#define VRGB_to_GRB(r, g, b) ((g << 16) + (r << 8) + (b))

//table 6.4Mhz to us (1bit)
//1 0.15625
//2 0.31250
//3 0.46875
//4 0.62500
//5 0.78125
//6 0.93750
//7 1.09375
//
//short state  	0.20 - 0.50 => 2, 3
//long state 	0.75 - 1.05 => 4, 5, 6,
//
//(logical 0) = Short 0 + Long 1
//2 + 6 => 11000000 => 0xCO
//
//(logical 1) = Long 1 + Short 0
//6 + 2 => 11111100 => 0xFC

#define CODE_0 0xC0
#define CODE_1 0xFC

uint32_t colorOrder[COLOR] = {0x800000, 0x808000, 0x008000, 0x008080, 0x000080};
uint32_t colors[LEDS]={0};
uint32_t k = 0;

void CT0_Callback(uint32_t flags)
{
	k++;

	if(k > (LEDS * COLOR))
	{
		k = 0;
	}
}

void Animate(void)
{
	for(int j=0;j<LEDS;j++)
	{
		colors[j] = 0x000000;
	}

	for(int i = 0; i < COLOR; i++)
	{
		for(int j = (i * 8); j < ((i + 1) * 8) && j < k; j++)
		{
			colors[j - (i * 8)]= HRGB_to_GRB(colorOrder[i]);
		}
	}
}

void Neopixels_Send(SPI_Type *base, uint32_t n, uint32_t *value)
{
	uint16_t LED_data = 0;
	uint32_t control = 0U;
	spi_config_t *g_config;
	uint32_t configFlags = 0U;

	//configFlags = kSPI_FrameAssert;
	g_config= SPI_GetConfig(base);

	/* set data width */
	control |= (uint32_t)SPI_FIFOWR_LEN((g_config->dataWidth));

	/* set sssel */
	control |= (SPI_DEASSERT_ALL & (~SPI_DEASSERTNUM_SSEL((uint32_t)(g_config->sselNum))));

	/* mask configFlags */
	control |= (configFlags & (uint32_t)SPI_FIFOWR_FLAGS_MASK);

	/* ignore RX */
	control |= (1 << 22);

	for(int j=0;j<n;j++)
	{
		for(int i=23;i>=0;i--)
		{
			LED_data = GET_BIT(value[j], i) ? CODE_1 : CODE_0;
			while(!(base->FIFOSTAT & kSPI_TxNotFullFlag)){};
			base->FIFOWR = LED_data | control;
		}
	}

	// Reset >= 50 us
	LED_data=0;
	for(int j=0;j<50;j++)
	{
			while(!(base->FIFOSTAT & kSPI_TxNotFullFlag)){};
			base->FIFOWR = LED_data | control;
	}
}

int main(void)
{
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();

	#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		/* Init FSL debug console. */
		BOARD_InitDebugConsole();
	#endif

	PRINTF("Hello world \r\n");

	while(1)
	{
		Animate();

		Neopixels_Send(FLEXCOMM8_PERIPHERAL, LEDS, colors);

		for(volatile int i=0;i<500000;i++);
	}

	return 0;
}
