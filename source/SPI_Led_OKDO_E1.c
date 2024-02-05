#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_debug_console.h"

//RGB LED Ring WS 28 12 B 5050 x 8 LEDs - 30mm

#define LEDS 8
#define TRANSFER_SIZE (24*LEDS) /*! Transfer dataSize */

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

uint32_t colors[LEDS]={0};
uint32_t k=0;

uint8_t masterRxData[TRANSFER_SIZE] = {0};
uint8_t masterTxData[TRANSFER_SIZE] = {0};

void Animate(void)
{

	int n=0;

	for(int j=0;j<LEDS;j++)
		colors[j]=0x000000;

	colors[k++]=HRGB_to_GRB(0xAA0000);


	if(k>=LEDS)
		k=0;

	n=0;
	for(int j=0;j<LEDS;j++)
	{
		for(int i=0;i<24;i++)
		{
			masterTxData[n]=GET_BIT(colors[j], 23-i) ? CODE_1 : CODE_0;
			n++;
		}
	}

}

int main(void)
{
	spi_transfer_t masterXfer;

	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();

	#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		/* Init FSL debug console. */
		BOARD_InitDebugConsole();
	#endif

		/*Config transfer*/
		masterXfer.txData = masterTxData;
		masterXfer.rxData = masterRxData;
		masterXfer.dataSize = sizeof(masterTxData);

		while(1)
		{
			Animate();
			SPI_MasterTransferBlocking(FLEXCOMM8_PERIPHERAL, &masterXfer);
			for(volatile int i=0;i<500000;i++);
		}

		return 0;
}
