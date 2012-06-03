/*
	ihsan Kehribar
	June 2012

	Attiny85 firmware for the tinyTouche project.
	https://github.com/kehribar/tinyTouche

	This project is based on Disney Touché 
	http://www.disneyresearch.com/research/projects/hci_touche_drp.htm

	The firmware is written from scratch, but the implementation idea
	comes from the Sprite_tm's engarde project.
	http://spritesmods.com/?art=engarde
		
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
			 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
					 
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <avr/io.h>
#include <util/delay.h>
#include "xitoa.h"
#include "suart.h"

#define sbi(register,bit) (register|=(1<<bit))
#define cbi(register,bit) (register&=~(1<<bit))

int main()
{
	uint8_t i;
	uint8_t temp=0;
	volatile uint8_t meas;
	const uint8_t step=128; // Data point number
	const uint8_t hop =256/step;

	xfunc_out = xmit;

	sbi(DDRB,4); /* Serial monitor output */
	sbi(DDRB,1); /* PWM output - OC1A */
	cbi(DDRB,2); /* ADC input */
	sbi(DIDR0,ADC1D); /* Disable digital port buffer at ADC1 pin */
		
	cbi(PLLCSR,LSM);
	sbi(PLLCSR,PLLE);
	while(bit_is_clear(PLLCSR,PLOCK));
	sbi(PLLCSR,PCKE);
	
	sbi(TCCR1,COM1A0); // toggle the pin
	sbi(TCCR1,CS10); 
	sbi(TCCR1,CTC1); // Clear at match
	
	sbi(ADMUX,MUX0);

	ADCSRA|= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	ADCSRA|=(1<<ADEN);

	xputs(PSTR("Reset!\n"));

	while(1)
	{
		OCR1C = 0; 
		temp = 0;
		xputs(PSTR("#"));
		for(i=0;i<step;i++)
		{			
			ADCSRA|=(1<<ADSC);
			while(ADCSRA & (1<<ADSC));
			temp += hop;
			OCR1C = temp;
			meas = ADCL;
			xputc(meas);
			meas = ADCH;			
			xputc(meas);
		}
		xputs(PSTR("#"));
		xputs(PSTR("\n"));
	}
	
	return 0;
}
