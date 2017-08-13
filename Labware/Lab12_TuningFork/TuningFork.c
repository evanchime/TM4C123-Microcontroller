// TuningFork.c Lab 12
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to create a squarewave at 440Hz.  
// There is a positive logic switch connected to PA3, PB3, or PE3.
// There is an output on PA2, PB2, or PE2. The output is 
//   connected to headphones through a 1k resistor.
// The volume-limiting resistor can be any value from 680 to 2000 ohms
// The tone is initially off, when the switch goes from
// not touched to touched, the tone toggles on/off.
//                   |---------|               |---------|     
// Switch   ---------|         |---------------|         |------
//
//                    |-| |-| |-| |-| |-| |-| |-|
// Tone     ----------| |-| |-| |-| |-| |-| |-| |---------------
//
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */


#include "TExaS.h"
#include "..//tm4c123gh6pm.h"

// Below are global variables to transfer data between the back/fore ground contexts
// SW_1ST_TIME, SW_2ND_TIME record the first / second times respectively the switch get pressed
// SW_RELEASED tells us the switch have been released after pressing first time
unsigned long SW_1ST_TIME, SW_2ND_TIME, SW_RELEASED;

// basic functions defined at end of startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode

// input from PA3, output from PA2, SysTick interrupts
void Sound_Init(void){ unsigned long volatile delay;
  SYSCTL_RCGC2_R |= 0x00000001; // activate port A
  delay = SYSCTL_RCGC2_R;
  GPIO_PORTA_AMSEL_R &= ~0x0C;      // no analog
  GPIO_PORTA_PCTL_R &= ~0x0000FF00; // regular function
  GPIO_PORTA_DIR_R &= ~0x08;     // make PA3 in
	GPIO_PORTA_DIR_R |= 0x04;     // make PA2 out
  GPIO_PORTA_DR8R_R |= 0x04;    // can drive up to 8mA out
  GPIO_PORTA_AFSEL_R &= ~0x0C;  // disable alt funct on PA3,2
  GPIO_PORTA_DEN_R |= 0x0C;     // enable digital I/O on PA3,2
  NVIC_ST_CTRL_R = 0;           // disable SysTick during setup
  NVIC_ST_RELOAD_R = 90908;     // reload value for 1.1363ms (assuming 80MHz)
  NVIC_ST_CURRENT_R = 0;        // any write to current clears it
  NVIC_SYS_PRI3_R = NVIC_SYS_PRI3_R&0x00FFFFFF; // priority 0 
  NVIC_ST_CTRL_R = 0x00000007;  // enable with core clock and interrupts	
}

// called at 880 Hz
void SysTick_Handler(void){
  if((SW_1ST_TIME == 0x01) && (SW_2ND_TIME == 0x00)) { // fisrt time switch pressed
	  GPIO_PORTA_DATA_R ^= 0x04; // toggle output
	}else{ // this will take care of if switches not pressed, or both switches now pressed
		GPIO_PORTA_DATA_R &= ~0x04; // set output to low
	  if(!(GPIO_PORTA_DATA_R & 0x08)){ // will check if  switch released 
			SW_1ST_TIME = SW_2ND_TIME = SW_RELEASED = 0x00; // will clear all global variables
		}	
	}
}

int main(void){// activate grader and set system clock to 80 MHz
  SW_1ST_TIME = 0x00, SW_2ND_TIME = 0x00, SW_RELEASED = 0x00;
	TExaS_Init(SW_PIN_PA3, HEADPHONE_PIN_PA2,ScopeOn); 
  Sound_Init();         
  EnableInterrupts();   // enable after all initialization are done
	while(1){  // main program is free to perform other tasks
    // do not use WaitForInterrupt() here, it may cause the TExaS to crash
		if (GPIO_PORTA_DATA_R & 0x08) { // this is to read switch, check if pressed first time
	    SW_1ST_TIME = 0x01; // global variable will be set recording first time switch pressed
    }
    if (!(GPIO_PORTA_DATA_R & 0x08) && SW_1ST_TIME) { // checks if switch have been released after first time pressed
	    SW_RELEASED = 0x01; // global variable will be set recording switch released after first time pressed
    }
    if ((GPIO_PORTA_DATA_R & 0x08) && SW_RELEASED ) { // this is to read switch, check if pressed second time
	    SW_2ND_TIME = 0x01; // global variable will be set recording second time switch pressed
    }  
	}
}
