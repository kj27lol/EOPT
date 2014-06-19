/*
 * ciaaUART.c
 *
 *  Created on: Jun 12, 2014
 *      Author: pablo
 */

#include "ciaaUART.h"

uint8_t uartRxBuf[UART_BUF_SIZE];
uint8_t uartTxBuf[UART_BUF_SIZE];
uint8_t * pTxIn = uartTxBuf;
uint8_t * pTxOut = uartTxBuf;
uint32_t uartRxDataCount = 0;

void ciaaUARTInit(void)
{
	UART_CFG_Type cfg;

	/* UART0 (RS485/Profibus) */
	cfg.Baud_rate = 115200;
	cfg.Clock_Speed = 0;
	cfg.Databits = UART_DATABIT_8;
	cfg.Parity = UART_PARITY_NONE;
	cfg.Stopbits = UART_STOPBIT_1;

	UART_Init(LPC_USART0, &cfg);
	UART_TxCmd(LPC_USART0, ENABLE);

	scu_pinmux(9, 5, MD_PDN, FUNC7); 					// P9_5: UART0_TXD
	scu_pinmux(9, 6, MD_PLN|MD_EZI|MD_ZI, FUNC7); 		// P9_6: UART0_RXD

	//DIR pin, software driven
	scu_pinmux(6, 2, MD_PDN, FUNC0); 					// P6_2: GPIO3[1]
	GPIO_SetDir(3, 1<<1, 1);
	GPIO_ClearValue(3, 1<<1);

	/* UART2 (USB-UART) */
	cfg.Baud_rate = 115200;
	cfg.Clock_Speed = 0;
	cfg.Databits = UART_DATABIT_8;
	cfg.Parity = UART_PARITY_NONE;
	cfg.Stopbits = UART_STOPBIT_1;

	UART_Init(LPC_USART2, &cfg);
	UART_TxCmd(LPC_USART2, ENABLE);

	scu_pinmux(7, 1, MD_PDN, FUNC6); 					// P7_1: UART2_TXD
	scu_pinmux(7, 2, MD_PLN|MD_EZI|MD_ZI, FUNC6); 		// P7_2: UART2_RXD

	UART_IntConfig(LPC_USART2, UART_INTCFG_RBR, ENABLE);

	NVIC_EnableIRQ(USART2_IRQn);

	/* UART3 (RS232) */
//	cfg.Baud_rate = 57600;
//	UART_Init(LPC_USART3, &cfg);
//	UART_TxCmd(LPC_USART3, ENABLE);
//
//	scu_pinmux(2, 3, MD_PDN, FUNC2); 					// P2_3: UART3_TXD
//	scu_pinmux(2, 4, MD_PLN|MD_EZI|MD_ZI, FUNC2); 		// P2_4: UART3_RXD
//
//	UART_IntConfig((LPC_USARTn_Type *)LPC_USART3, UART_INTCFG_RBR, ENABLE);
//	NVIC_EnableIRQ(USART3_IRQn);

}

void UART2_IRQHandler(void)
{
	uint8_t status = UART_GetLineStatus(LPC_USART2);

	if(status&UART_LINESTAT_RDR)
	{
		uartRxBuf[uartRxDataCount] = UART_ReceiveByte(LPC_USART2);
		uartRxDataCount++;
	}
	if(status&UART_LINESTAT_THRE)
	{
		pTxOut++;
		if(pTxOut == (uartTxBuf + UART_BUF_SIZE))
			pTxOut = uartTxBuf;
		if(pTxIn == pTxOut)
		{
			UART_IntConfig(LPC_USART2, UART_INTCFG_THRE, DISABLE);
			return;
		}
		UART_SendByte(LPC_USART2, *pTxOut);
	}
}

void UART3_IRQHandler(void)
{

}

void uartSend(void * data, int datalen)
{
	int i;

	if(datalen==0) return;

	for(i=0; i<datalen; i++)
	{
		*pTxIn = ((uint8_t*)data)[i];

		if(pTxIn == pTxOut)
		{
			UART_SendByte(LPC_USART2, *pTxOut);
			UART_IntConfig(LPC_USART2, UART_INTCFG_THRE, ENABLE);
		}

		pTxIn++;
		if(pTxIn == (uartTxBuf + UART_BUF_SIZE))
			pTxIn = uartTxBuf;
	}
}

void ciaaUART485Send(void * data, int datalen)
{
	int foo;
	GPIO_SetValue(3, 1<<1);
	for(foo=0; foo<0xFFF; foo++);
	UART_Send(LPC_USART0, (uint8_t *)data, datalen, BLOCKING);
	for(foo=0; foo<0xFFF; foo++);
	GPIO_ClearValue(3, 1<<1);
}

void ciaaUART485Recv(void * data, int datalen)
{
	UART_Receive(LPC_USART0, (uint8_t *)data, datalen, BLOCKING);
}
