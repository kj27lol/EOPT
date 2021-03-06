/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC43xx.h"
#endif


#include <cr_section_macros.h>

#include "cr_start_m0.h"

#include "lpc43xx_gpio.h"
#include "lpc43xx_scu.h"
#include "lpc43xx_cgu.h"
#include "lpc43xx_rit.h"
#include "lpc43xx_uart.h"
#include "debug_frmwrk.h"

#include "webserver.h"
#include "tcpip.h"
#include "database.h"

#include "stdio.h"

#include "ciaaUART.h"
#include "ciaaI2C.h"
#include "ciaaIO.h"
#include "ciaaAOUT.h"
#include "ciaaAIN.h"

#include "vcomdemo.h"

volatile uint32_t msec;

uint32_t wiegandTimeout;

#define wiegandData0() (GPIO_ReadValue(3)&(1<<0))
#define wiegandData1() (GPIO_ReadValue(5)&(1<<5))

void pausems(uint32_t t)
{
	msec = t;
	while(msec) __WFI();
}

uint32_t wiegand26(void)
{
	uint32_t word = 0;
	int bit;

	for(bit = 0; bit<26; bit++)
	{
		//Espero por un 0 en Data0 o Data1
		wiegandTimeout = 10;
		while(wiegandData0() && wiegandData1() && wiegandTimeout);

		if(wiegandTimeout==0) return 0;

		//Si el 0 viene por Data1, poner el bit en 1
		if(!wiegandData1())
			word |= 1<<bit;

		//Espero por Data0 y Data1 en cero
		while(!(wiegandData0()&&wiegandData1()));
	}

	return word;
}

void wiegandReaderTask(void)
{
	char uid[20];

	uint32_t word = wiegand26();

	if(word == 0) return;

	sprintf(uid, "%08X", (unsigned int)word);
	dbNewUID(uid);

	dbgPrint("[wiegandReaderTask]Tag leido: ");
	dbgPrint(uid);
	dbgPrint("\n");

}

void SysTick_Handler (void) 					// SysTick Interrupt Handler @ 1000Hz
{
	static int TimeTick=0;

	if(msec)msec--;
	if(wiegandTimeout) wiegandTimeout--;

	TimeTick++;
	if (TimeTick >= 200)
	{
		TimeTick = 0;
		TCPClockHandler();
	}

	if(wiegandData0() && wiegandData1()) return;

}

void setupHardware(void)
{
//	Vcom_init();

    SysTick_Config(CGU_GetPCLKFrequency(CGU_PERIPHERAL_M4CORE)/1000);

//
//	ciaaAOUTInit();
//
//	ciaaAINInit();
//
//	ciaaIOInit();

	ciaaUARTInit();

	uartSend("test\n", 5);

	while(1);

//	int i = 100, flag = 0;
//	char str[100];
//	while(i != 1)
//	{
//		ciaaAOUTSet(i);
//		i -= 10;
//		if(i<0) i = 100;
//		pausems(2000);
//
//		if(flag == 0)
//		{
//			GPIO_ClearValue(1,(1<<8));
//			flag = 1;
//		}
//		else
//		{
//			GPIO_SetValue(1,(1<<8));
//			flag = 0;
//		}
//
//		sprintf(str, "%d %d %d %d\n",
//				ciaaAINRead(0), ciaaAINRead(1),
//				ciaaAINRead(2), ciaaAINRead(3));
//		dbgPrint(str);
//	}

    scu_pinmux(4 , 8, MD_PUP, FUNC4); 	// P8.1 : USB0_IND1 LED
	GPIO_SetDir(LED1_PORT,(1<<LED1_BIT), 1);
	GPIO_SetValue(LED1_PORT,(1<<LED1_BIT));

	RIT_Init(LPC_RITIMER);
	RIT_TimerConfig(LPC_RITIMER, 1);
	RIT_Cmd(LPC_RITIMER, ENABLE);


	ciaaI2CInit();

	enetInit();
}

void mem48Read(uint8_t addr, void * buffer, int len)
{
	int i;

	/* Primero escribo dirección: 1 byte */
	ciaaI2CWrite(0x51, &addr, 1);

	//tiempo de establecimiento de la memoria
	//for(i=0; i<0xFFFF; i++);

	/* Leo byte (lectura propiamente dicha) */
	ciaaI2CRead(0x51, buffer, len);

	//tiempo de establecimiento de la memoria
	for(i=0; i<0xFFFF; i++);
}

void mem48Write(uint8_t addr, void * buffer, int len)
{
	uint8_t * pdatos = (uint8_t *)buffer;
	uint8_t buf[34];
	int i;

	while(len>0)
	{
		buf[0] = addr;

		for(i=0; i < ( len > 32 ? 32 : len ); i++)
			buf[i+1] = pdatos[i];

		ciaaI2CWrite(0x51, buf, len > 32 ? 34 : len+2);
		//tiempo de establecimiento de la memoria
		for(i=0; i<0xFFFF; i++);

		addr += 32;
		len -= 32;
		pdatos+=32;
	}
}

void memRead(uint16_t addr, void * buffer, int len)
{
	unsigned char txbuf[2];
	int i;

	txbuf[0] = addr >> 8;
	txbuf[1] = addr & 0xFF;

	/* Primero escribo dirección: 2 bytes */
	ciaaI2CWrite(0x50, txbuf, 2);

	//tiempo de establecimiento de la memoria
	//for(i=0; i<0xFFFF; i++);

	/* Leo byte (lectura propiamente dicha) */
	ciaaI2CRead(0x50, buffer, len);

	//tiempo de establecimiento de la memoria
	for(i=0; i<0xFFFF; i++);
}

void memWrite(uint16_t addr, void * buffer, int len)
{
	uint8_t * pdatos = (uint8_t *)buffer;
	uint8_t buf[34];
	int i;

	while(len>0)
	{
		buf[0] = addr >> 8;
		buf[1] = addr & 0xFF;

		for(i=0; i < ( len > 32 ? 32 : len ); i++)
			buf[i+2] = pdatos[i];

		ciaaI2CWrite(0x50, buf, len > 32 ? 34 : len+2);
		//tiempo de establecimiento de la memoria
		for(i=0; i<0xFFFF; i++);

		addr += 32;
		len -= 32;
		pdatos+=32;
	}
}

int main(void)
{
    cr_start_m0(SLAVE_M0APP,&__core_m0app_START__);

 	setupHardware();

	dbgPrint("[main]CIAA Demo 1.0\n");

	char txbuf[3] = {0x55,0xF0,0xAA};
	char rxbuf[3] = {0,0,0};
	char rxbuf48[3] = {0,0,0};

	memWrite(0, txbuf, 3);

	memRead(0, rxbuf, 3);

	mem48Write(0, txbuf, 3);
	mem48Read(0, rxbuf48, 3);

	uint8_t uid48[6] = {0,0,0,0,0,0};

	mem48Read(0xFA, uid48, 6);


#if 0
    msec = 0;
    do
    {
    	uartRxDataCount=0;
		UARTPuts(LPC_USART3, "x");
		msec=500;
    	while(msec);
    }while(uartRxBuf[0]!='M');

    UARTPuts(LPC_USART3, ".");
#endif

	while(1)
	{
		webPeriodicalTask();

		wiegandReaderTask();
#if 0
		if(msec==0)
		{
			uartRxDataCount=0;
			UARTPuts(LPC_USART3, "s");
			msec=1000;
		}

		if(uartRxDataCount && (uartRxBuf[0]=='N'))
			uartRxDataCount=0;
		if(uartRxDataCount==8)
		{
			dbNewUID((char *)uartRxBuf);
			uartRxDataCount=0;
		}
#endif
	}
}

void check_failed(uint8_t *file, uint32_t line)
{
	while(1);
}

