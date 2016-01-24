/*
  ihsan Kehribar
  June 2012 - Initial commit
  January 2016 - Major update

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
  uint8_t meas_l;  
  uint8_t meas_h;  
  uint8_t checksum;
  
  xfunc_out = xmit;

  /* Serial monitor output */
  sbi(DDRB,3);

  /* LED output */
  sbi(DDRB,4);  

   /* PWM output - OC1A */
  sbi(DDRB,1);
  
  /* ADC input */
  cbi(DDRB,2);
  
  /* Disable digital port buffer at ADC2 pin */
  sbi(DIDR0,ADC2D);
    
  /* Set PLL mode for high speed PWM output */    
  cbi(PLLCSR,LSM);
  sbi(PLLCSR,PLLE);
  while(bit_is_clear(PLLCSR,PLOCK));
  sbi(PLLCSR,PCKE);
  
  /* Set PWM module */
  sbi(TCCR1,COM1A0);
  sbi(TCCR1,CS10); 
  sbi(TCCR1,CTC1);
  
  /* Set ADC */
  sbi(ADMUX,MUX0);
  ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
  ADCSRA |= (1<<ADEN);

  while(1)
  {         
    OCR1C = 0;   
    _delay_us(250);

    checksum = 0;
    
    /* Send SYNC word */
    xputc(0xDE);
    xputc(0xAD);
    xputc(0xBE);
    xputc(0xEF);    
        
    /* Toggle LED */
    sbi(PINB,4);      

    /* Trigger an ADC conversion */ 
    ADCSRA |= (1<<ADSC);

    /* Wait until conversion is finished */
    while(ADCSRA & (1<<ADSC));      
  
    for(i=1;i<151;i++)
    { 
      meas_l = ADCL;              
      meas_h = ADCH;         
      
      /* Trigger an ADC conversion */ 
      ADCSRA |= (1<<ADSC);
          
      /* Avoid special '\r' and '\n' values for lower part of the result */      
      /* This is a bug in the host side software, but fixed here ... */
      if(meas_l == 0x0D)
      {
        meas_l = 0x0E;      
      }
      else if(meas_l == 0x0A)
      {
        meas_l = 0x0B;      
      }      

      /* Send conversion results */
      xputc(meas_l);      
      xputc(meas_h);      

      /* Update checksum */
      checksum += meas_l;
      checksum += meas_h;      

      /* Wait until conversion is finished */
      while(ADCSRA & (1<<ADSC));      

      /* Update output frequency */
      OCR1C = i;    
    }   

    /* Send the checksum last */
    xputc(checksum);
  }
  
  return 0;
}
