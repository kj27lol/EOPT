/*
===============================================================================
 Name        : ciaaTest.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include "chip.h"
#include "ciaaIO.h"

#include <cr_section_macros.h>

void SysTick_Handler(void)
{
	volatile static int cont=0;

	cont++;

	if(cont==250)
	{
		ciaaWriteOutput(5, 1);
	}
	if(cont==500)
	{
		ciaaWriteOutput(5, 0);
		cont = 0;
	}
}

int main(void)
{
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000);

	ciaaIOInit();

	while(1) __WFI();

}