/*
 * ciaaInputs.c
 *
 *  Created on: Jun 12, 2014
 *      Author: pablo
 */

#include "ciaaIO.h"

void ciaaIOInit(void)
{
	/* Inputs  */
	scu_pinmux(4,0,MD_PUP|MD_EZI|MD_ZI,FUNC0);	//GPIO2[0]
	scu_pinmux(4,1,MD_PUP|MD_EZI|MD_ZI,FUNC0);	//GPIO2[1]
	scu_pinmux(4,2,MD_PUP|MD_EZI|MD_ZI,FUNC0);	//GPIO2[2]
	scu_pinmux(4,3,MD_PUP|MD_EZI|MD_ZI,FUNC0);	//GPIO2[3]
	scu_pinmux(7,3,MD_PUP|MD_EZI|MD_ZI,FUNC0);	//GPIO3[11]
	scu_pinmux(7,4,MD_PUP|MD_EZI|MD_ZI,FUNC0);	//GPIO3[12]
	scu_pinmux(7,5,MD_PUP|MD_EZI|MD_ZI,FUNC0);	//GPIO3[13]
	scu_pinmux(7,6,MD_PUP|MD_EZI|MD_ZI,FUNC0);	//GPIO3[14]

	GPIO_SetDir(2,0xF, 0);
	GPIO_SetDir(3, 0xF<<11, 0);

	/* MOSFETs */
	scu_pinmux(4,8,MD_PUP,FUNC4); //GPIO5[12]
	scu_pinmux(4,9,MD_PUP,FUNC4);//GPIO5[13]
	scu_pinmux(4,10,MD_PUP,FUNC4);//GPIO5[14]
	scu_pinmux(1,5,MD_PUP,FUNC0);//GPIO1[8]
	GPIO_SetDir(5,(1<<12)|(1<<13)|(1<<14),1);
	GPIO_SetDir(1,(1<<8),1);

	GPIO_SetValue(5,(1<<12)|(1<<13)|(1<<14));
	GPIO_SetValue(1,(1<<8));

	/* Relays */
	GPIO_SetDir(2,(1<<4)|(1<<5)|(1<<6),1);
	scu_pinmux(2,1,MD_PUP,FUNC4);
	GPIO_SetDir(5,(1<<1),1);

	GPIO_ClearValue(2,(1<<4)|(1<<5)|(1<<6));
	GPIO_ClearValue(5,(1<<1));
}

uint32_t ciaaReadInput(uint32_t inputNumber)
{
	if(inputNumber > 7) return ~0;
	return ciaaDigitalInputs() & (1<<inputNumber);
}

uint32_t ciaaWriteOutput(uint32_t outputNumber, uint32_t value)
{
	/* TODO */
	return 0;
}
