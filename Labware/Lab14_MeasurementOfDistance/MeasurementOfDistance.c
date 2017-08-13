// MeasurementOfDistance.c
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to periodically initiate a software-
// triggered ADC conversion, convert the sample to a fixed-
// point decimal distance, and store the result in a mailbox.
// The foreground thread takes the result from the mailbox,
// converts the result to a string, and prints it to the
// Nokia5110 LCD.  The display is optional.
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

// Slide pot pin 3 connected to +3.3V
// Slide pot pin 2 connected to PE2(Ain1) and PD3
// Slide pot pin 1 connected to ground


#include "ADC.h"
#include "..//tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "TExaS.h"

void EnableInterrupts(void);  // Enable interrupts

unsigned char String[10]; // null-terminated ASCII string
unsigned long Distance;   // units 0.001 cm
unsigned long ADCdata;    // 12-bit 0 to 4095 sample
unsigned long Flag;       // 1 means valid Distance, 0 means Distance is empty

//********Convert****************
// Convert a 12-bit binary ADC sample into a 32-bit unsigned
// fixed-point distance (resolution 0.001 cm).  Calibration
// data is gathered using known distances and reading the
// ADC value measured on PE1.  
// Overflow and dropout should be considered 
// Input: sample  12-bit ADC sample
// Output: 32-bit distance (resolution 0.001cm)
unsigned long Convert(unsigned long sample){ // function governed by Distance = (A*ADCdata)>>10 + B, 
	// where Distance is returned value, A,B are calibration constants, ADCdata is the sample
	sample = 0.5006 * sample; // A*ADCdata
	return ((sample *1000) >> 10) + 0; // B = 0 , 1000 accounts for resolution 0.001cm
  //return ((sample * 1338) >> 10) + 281;  // real board code only
}

// Initialize SysTick interrupts to trigger at 40 Hz, 25 ms
void SysTick_Init(unsigned long period){
	NVIC_ST_CTRL_R = 0;           // disable SysTick during setup
  NVIC_ST_RELOAD_R = period - 1;     // reload value (assuming 80MHz)
  NVIC_ST_CURRENT_R = 0;        // any write to current clears it
  NVIC_SYS_PRI3_R = NVIC_SYS_PRI3_R&0x00FFFFFF; // priority 0 
  NVIC_ST_CTRL_R = 0x00000007;  // enable with core clock and interrupts
}
// executes every 25 ms, collects a sample, converts and stores in mailbox
void SysTick_Handler(void){
  GPIO_PORTF_DATA_R ^= 0x02; // Toggle PF1 
  GPIO_PORTF_DATA_R ^= 0x02; // Toggle PF1 again
  ADCdata = ADC0_In(); // Sample the ADC, calling ADC0_In() and storing in global variable ADCdata
  Distance = Convert(ADCdata); // Convert the sample to Distance, calling Convert(), and storing the result in the global, Distance 
  Flag = 1; // Set the Flag, signifying new data is ready 
  GPIO_PORTF_DATA_R ^= 0x02; // Toggle PF1 a third time
}

//-----------------------UART_ConvertDistance-----------------------
// Converts a 32-bit distance into an ASCII string
// Input: 32-bit number to be converted (resolution 0.001cm)
// Output: store the conversion in global variable String[10]
// Fixed format 1 digit, point, 3 digits, space, units, null termination
// Examples
//    4 to "0.004 cm"  
//   31 to "0.031 cm" 
//  102 to "0.102 cm" 
// 2210 to "2.210 cm"
//10000 to "*.*** cm"  any value larger than 9999 converted to "*.*** cm"
void UART_ConvertDistance(unsigned long n){
// as part of Lab 11 you implemented this function
	char iii = 0, jjj = 4, buffer[10];// buffer will hold ASCII in reverse order. iii, jjj are loop control variables.  
// jjj starts at 4, because buffer's index 0,1,2,3 will be null, m, c and space character respectively
	if (n < 10000) {// if n > 9999
// next line takes care of ending null termination, m ,c , and space characters
		buffer[0] = '\0', buffer[1] = 'm', buffer[2] = 'c', buffer[3] = ' ';
		for (iii = 4; iii < 10; ) {// populates part of buffer with zero
			buffer[iii++] = 0x30;
		}
		do {
			buffer[jjj++] = 0x30 + n % 10;  // this is to get the least significant digit
			n = n / 10; // these is to reduce n eventually to zero
		} while (n);
// next line will store buffer contents in String, 
// taking care of the required decimal point
		for (jjj = 7, iii = 0; jjj >= 0;) {
			if (jjj == 7) {
				String[iii++] = buffer[jjj--];
				String[iii++] = '.';
			}
			else {
				String[iii++] = buffer[jjj--];
			}
		}
	}
	else{// case where n > 9999
		for (;iii < 5;) { // populates part of buffer with * character
			String[iii++] = '*';
		}
		String[1] = '.', String[5] = ' ', String[6] = 'c', String[7] = 'm'; // takes care of the required end characters
	}
}

// main1 is a simple main program allowing you to debug the ADC interface
int main1(void){ 
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
  EnableInterrupts(); // enable interrupts
  while(1){ 
    ADCdata = ADC0_In(); // debug converted ADC sample
  }
}
// once the ADC is operational, you can use main2 to debug the convert to distance
int main2(void){ 
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_NoScope);
  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
  Nokia5110_Init();             // initialize Nokia5110 LCD
  EnableInterrupts(); // enable interrupts
  while(1){ 
    ADCdata = ADC0_In(); // debug converted ADC sample
    Nokia5110_SetCursor(0, 0); //  set nokia lcd to default. debugging
    Distance = Convert(ADCdata); //  convert ADC sample to distance. debugging
    UART_ConvertDistance(Distance); // then convert Distance to String. debugging
    Nokia5110_OutString(String);    // output to Nokia5110 LCD (optional). debugging
  }
}
// once the ADC and convert to distance functions are operational,
// you should use this main to build the final solution with interrupts and mailbox
int main(void){ 
  volatile unsigned long delay;
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
  Nokia5110_Init();             // initialize Nokia5110 LCD
  SysTick_Init(2000000); // initialize SysTick for 40 Hz interrupts
// initialize profiling on PF1 (optional)
	SYSCTL_RCGC2_R |= 0x00000020;     // 1) F clock
  delay = SYSCTL_RCGC2_R;           // delay   	
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTF_DIR_R |= 0x02;          // 5) make PF1 output   
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function	
  GPIO_PORTF_DEN_R |= 0x02;         // 7) enable digital pins PF1
	EnableInterrupts(); // enable interrupts
// print a welcome message  (optional)
  while(1){ 
		Flag = 0; // clear flag to make sure flag starts afresh
		while(!Flag); // wait for flag to be set, by systick handler
		UART_ConvertDistance(Distance); //  then convert Distance to String
    Nokia5110_OutString(String); // and output to Nokia5110 LCD (optional)
  }
	
	
}
