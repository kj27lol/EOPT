/*
 * ciaaSPIFI.c
 *
 *  Created on: Jun 30, 2014
 *      Author: pablo
 */

#include "ciaaSPIFI.h"

typedef struct
{
	volatile uint32_t CTRL;
	volatile uint32_t CMD;
	volatile uint32_t ADDR;
	volatile uint32_t DATINTM;
	volatile uint32_t CACHELIMIT;
	volatile uint32_t DAT;
	volatile uint32_t MEMCMD;
	volatile uint32_t STAT;
} LPC_SPIFI_TypeDef;

LPC_SPIFI_TypeDef * LPC_SPIFI = (LPC_SPIFI_TypeDef*)0x40003000;

void spifi_setup(void)
{
	SCnSCB->ACTLR &= ~2; // disable Cortex write buffer to avoid exceptions when switching back to SPIFI for execution

	LPC_SPIFI->STAT = 0x10;	// reset memory mode
	while(LPC_SPIFI->STAT & 0x10);	// wait for reset to complete

	LPC_SPIFI->ADDR = 0xffffffff;
	LPC_SPIFI->DATINTM = 0xffffffff;
	LPC_SPIFI->CMD = // send all ones for a while to hopefully reset SPI Flash
		(0xfful << 24) | // opcode 0xff
		(0x5 << 21) | // frame form indicating opcode and 4 address bytes
		(0x0 << 19) | // field form indicating all serial fields
		(0x4 << 16); // 3 intermediate data bytes
	while(LPC_SPIFI->STAT & 2); // wait for command to complete

	LPC_SPIFI->CTRL = // set up new CTRL register with high speed options
		(0x100 << 0) | // timeout
		(0x1 << 16) | // cs high, this parameter is dependent on the SPI Flash part and is in SPIFI_CLK cycles. A lower cs high performs faster
		(1 << 29) | // receive full clock (rfclk) - allows for higher speeds
		(1 << 30); // feedback clock (fbclk) - allows for higher speeds

	// put part in high speed mode (skipping opcodes)
	LPC_SPIFI->DATINTM = 0xa5; // 0xAx will cause part to use high performace reads (skips opcode on subsequent accesses)
	LPC_SPIFI->CMD =
		(0xecul << 24) | // opcode 0xec Quad IO High Performance Read for Spansion
		(0x5 << 21) | // frame form indicating opcode and 4 address bytes
		(0x2 << 19) | // field form indicating serial opcode and dual/quad other fields
		(0x3 << 16); // 3 intermediate data bytes
	while(LPC_SPIFI->STAT & 2); // wait for command to complete

	LPC_SPIFI->MEMCMD =
		(0xecul << 24) | // opcode 0xeb Quad IO High Performance Read for Spansion
		(0x7 << 21) | // frame form indicating no opcode and 4 address bytes
		(0x2 << 19) | // field form indicating serial opcode and dual/quad other fields
		(0x3 << 16) ;// 3 intermediate data bytes

	SCnSCB->ACTLR |= 2; // reenable Cortex write buffer

}

char quad = 1;
int copy[1024];
int comp[1024];

void spifi_update_const(int address, int value) {
	int aligned_address = address & ~(0xfff);
	int offset = address & 0xfff;
	int i, j;

	SCnSCB->ACTLR &= ~2; // disable Cortex write buffer to avoid exceptions when switching back to SPIFI for execution

	LPC_SPIFI->STAT = 0x10;	// reset memory mode
	while(LPC_SPIFI->STAT & 0x10);	// wait for reset to complete

	LPC_SPIFI->DATINTM = 0x0; // next read command will remove high performance mode
	LPC_SPIFI->ADDR = aligned_address;
	LPC_SPIFI->CMD =
		(0xecul << 24) | // opcode 0xec Quad IO High Performance Read for Spansion
		(0x7 << 21) | // frame form indicating no opcode and 4 address bytes
		(0x2 << 19) | // field form indicating serial opcode and dual/quad other fields
		(0x3 << 16) | // 3 intermediate data bytes
		(4096); // datalen
	for(i = 0; i < 1024; i++)
		copy[i] = *(volatile int*)&LPC_SPIFI->DAT;
	while(LPC_SPIFI->STAT & 2); // wait for command to complete

	//hasta acá parece que lee el sector correctamente

	//Write Enable
	LPC_SPIFI->CMD =
		(0x06ul << 24) | // opcode 0x06 Write Enable for Spansion
		(0x1 << 21) | // frame form indicating opcode only
		(0x0 << 19); // field form indicating all serial
	while(LPC_SPIFI->STAT & 2); // wait for command to complete

	//Erase sector
	LPC_SPIFI->ADDR = aligned_address;
	LPC_SPIFI->CMD =
		(0x21ul << 24) | // opcode 0x21 Sector Erase for Spansion
		(0x5 << 21) | // frame form indicating opcode and 4 address bytes
		(0x0 << 19); // field form indicating all serial
	while(LPC_SPIFI->STAT & 2); // wait for command to complete

	//Read again (para probar)
	LPC_SPIFI->DATINTM = 0x0; // next read command will remove high performance mode
	LPC_SPIFI->ADDR = aligned_address;
	LPC_SPIFI->CMD =
		(0xecul << 24) | // opcode 0xec Quad IO High Performance Read for Spansion
		(0x7 << 21) | // frame form indicating no opcode and 4 address bytes
		(0x2 << 19) | // field form indicating serial opcode and dual/quad other fields
		(0x3 << 16) | // 3 intermediate data bytes
		(4096); // datalen
	for(i = 0; i < 1024; i++)
		copy[i] = *(volatile int*)&LPC_SPIFI->DAT;
	while(LPC_SPIFI->STAT & 2); // wait for command to complete

	int * p = (int *)0x14000000;
	for(i=0 ;i<1024; i++, p++)
		comp[i] = *p;


	while(1);



	LPC_SPIFI->CMD =
		(0xd8ul << 24) | // opcode 0x20 Sector Erase for Spansion
		(0x4 << 21) | // frame form indicating opcode and 3 address bytes
		(0x0 << 19); // field form indicating all serial
	while(LPC_SPIFI->STAT & 2); // wait for command to complete
	LPC_SPIFI->CMD =
		(0x05ul << 24) | // opcode 0x05 Read Status for Spansion
		(0x1 << 21) | // frame form indicating opcode only
		(0x0 << 19) | // field form indicating all serial
		(0x1 << 14) | // POLLRS polling command
		(0x0 << 2) | // check bit 0
		(0x0 << 0); // wait till 0
	while(LPC_SPIFI->STAT & 2); // wait for command to complete
	*(volatile char*)&LPC_SPIFI->DAT;




	copy[offset/4] = value;

	for(j = 0; j < 1024; j += 64) {
		LPC_SPIFI->CMD =
			(0x06ul << 24) | // opcode 0x06 Write Enable for Spansion
			(0x1 << 21) | // frame form indicating opcode only
			(0x0 << 19); // field form indicating all serial
		while(LPC_SPIFI->STAT & 2); // wait for command to complete
		LPC_SPIFI->CMD =
			(0x32ul << 24) | // opcode 0x32 Quad Page Programming for Spansion
			(0x4 << 21) | // frame form indicating opcode and 3 address bytes
			(0x1 << 19) | // field form indicating quad data field, others serial
			(0x0 << 16) | // 0 intermediate data bytes
			(0x1 << 15) | // dout indicates that it is a write
			(256); // datalen
		for(i = 0; i < 64; i++)
			*(volatile int*)&LPC_SPIFI->DAT = copy[j + i];
		while(LPC_SPIFI->STAT & 2); // wait for command to complete
		LPC_SPIFI->CMD =
			(0x05ul << 24) | // opcode 0x05 Read Status for Spansion
			(0x1 << 21) | // frame form indicating opcode only
			(0x0 << 19) | // field form indicating all serial
			(0x1 << 14) | // POLLRS polling command
			(0x0 << 2) | // check bit 0
			(0x0 << 0); // wait till 0
		while(LPC_SPIFI->STAT & 2); // wait for command to complete
		*(volatile char*)&LPC_SPIFI->DAT;
		LPC_SPIFI->ADDR += 256;
	}

	// put part in high speed mode (skipping opcodes)
	LPC_SPIFI->DATINTM = 0xa5; // 0xAx will cause part to use high performace reads (skips opcode on subsequent accesses)
	LPC_SPIFI->CMD =
		(0xecul << 24) | // opcode 0xeb Quad IO High Performance Read for Spansion
		(0x5 << 21) | // frame form indicating opcode and 4 address bytes
		(0x2 << 19) | // field form indicating serial opcode and dual/quad other fields
		(0x3 << 16); // 3 intermediate data bytes
	while(LPC_SPIFI->STAT & 2); // wait for command to complete

	LPC_SPIFI->MEMCMD =
		(0xecul << 24) | // opcode 0xeb Quad IO High Performance Read for Spansion
		(0x7 << 21) | // frame form indicating no opcode and 4 address bytes
		(0x2 << 19) | // field form indicating serial opcode and dual/quad other fields
		(0x3 << 16); // 3 intermediate data bytes

	SCnSCB->ACTLR |= 2; // reenable Cortex write buffer
}

/* SPIFI high speed pin mode setup */
STATIC const PINMUX_GRP_T spifipinmuxing[] = {
	{0x3, 3,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI CLK */
	{0x3, 4,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D3 */
	{0x3, 5,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D2 */
	{0x3, 6,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D1 */
	{0x3, 7,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)},	/* SPIFI D0 */
	{0x3, 8,  (SCU_PINIO_FAST | SCU_MODE_FUNC3)}	/* SPIFI CS/SSEL */
};

int ciaaSPIFIInit(void)
{
	static uint32_t data;

	/* SPIFI pin setup is done prior to setting up system clocking */
	Chip_SCU_SetPinMuxing(spifipinmuxing, sizeof(spifipinmuxing) / sizeof(PINMUX_GRP_T));
	Chip_Clock_SetBaseClock(CLK_BASE_SPIFI, CLKIN_IDIVE, true, false);


	spifi_setup();

	LPC_SPIFI->CMD =
		(0x90ul << 24) | //
		(0x4 << 21) | // frame form indicating opcode and 3 address bytes
		(0x0 << 19) | //
		(0x0 << 16) | // 0 intermediate data bytes
		(0x0 << 15) | //
		(4); // datalen

	data = *(volatile int*)&LPC_SPIFI->DAT;

	while(LPC_SPIFI->STAT & 2); // wait for command to complete

	spifi_update_const(0, 1234567890);

	spifi_update_const(4, 987654321);

	data = *(int *)0x14000000;

	return data;
}
