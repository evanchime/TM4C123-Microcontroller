// ***** 0. Documentation Section *****
// SwitchLEDInterface.c for Lab 8
// Runs on LM4F120/TM4C123
// Use simple programming structures in C to toggle an LED
// while a button is pressed and turn the LED on when the
// button is released.  This lab requires external hardware
// to be wired to the LaunchPad using the prototyping board.
// January 15, 2016
//      Jon Valvano and Ramesh Yerraballi

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"

#define GPIO_PORTE_DATA_R       (*((volatile unsigned long *)0x400243FC))
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_PUR_R        (*((volatile unsigned long *)0x40024510))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_LOCK_R       (*((volatile unsigned long *)0x40024520))
#define GPIO_PORTE_CR_R         (*((volatile unsigned long *)0x40024524))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))

// ***** 2. Global Declarations Section *****

// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void); // Enable interrupts
void PortE_Init(void); // Initialize PORT E
void LED_ON(void); // Function to turn LED on
void Delay1ms(unsigned long msec);  // Delay function
void Toggle_LED(void); // Toggle LED function
void Read_Switch_Decide(void); //  function to Read switch and make decision

// ***** 3. Subroutines Section *****

// PE0, PB0, or PA2 connected to positive logic momentary switch using 10k ohm pull down resistor
// PE1, PB1, or PA3 connected to positive logic LED through 470 ohm current limiting resistor
// To avoid damaging your hardware, ensure that your circuits match the schematic
// shown in Lab8_artist.sch (PCB Artist schematic file) or 
// Lab8_artist.pdf (compatible with many various readers like Adobe Acrobat).
int main(void){ 
//**********************************************************************
// The following version tests input on PE0 and output on PE1
//**********************************************************************
  TExaS_Init(SW_PIN_PE0, LED_PIN_PE1, ScopeOn);  // activate grader and set system clock to 80 MHz
	PortE_Init(); // initialise PE1,PE0
  EnableInterrupts();           // enable interrupts for the grader
	LED_ON(); // turn LED on
  
	while(1){
	  Delay1ms(100); // Delay 100ms
    Read_Switch_Decide();	// Reads switch and makes decision	
    }
}

// Subroutine to initialize port E pins for input and output
// PE0 is input Switch and PE1 is output LED
// Inputs: None
// Outputs: None
// Notes: ...
void PortE_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000010;      // 1) E clock
  delay = SYSCTL_RCGC2_R;            // delay to allow clock to stabilize     
  GPIO_PORTE_AMSEL_R &= 0x00;        // 2) disable analog function
  GPIO_PORTE_PCTL_R &= 0x00000000;   // 3) GPIO clear bit PCTL  
  GPIO_PORTE_DIR_R &= ~0x01;         // 4.1) PE0 input,
  GPIO_PORTE_DIR_R |= 0x02;          // 4.2) PE1 output  
  GPIO_PORTE_AFSEL_R &= 0x00;        // 5) no alternate function      
  GPIO_PORTE_DEN_R |= 0x03;          // 7) enable digital pins PE1,PE0
}

// Subroutine to turn LED(PE1) on 
// Inputs:  None
// Outputs: None
// Notes:...............
void LED_ON(void){
  GPIO_PORTE_DATA_R |= 0x02; // turn LED on
}

// Subroutine to delay in units of milliseconds
// Inputs:  Number of milliseconds to delay
// Outputs: None
// Notes:   assumes 80 MHz clock
void Delay1ms(unsigned long msec){
    unsigned long i;
	  while(msec > 0){
			i = 16000;  // i in simulation
			// i = (16000*25/30); //  i in the real board
			while(i > 0){
				i = i - 1;
			}
      msec = msec - 1;
    }
}

// Subroutine to toggle LED
// Inputs:  None
// Outputs: None
// Notes:...............
void Toggle_LED(void){
  GPIO_PORTE_DATA_R  ^= 0x02; // Toggle LED
}	

// Subroutine to read switch then decide to toggle or make LED on
// Inputs:  None
// Outputs: None
// Notes:...............
void Read_Switch_Decide(void){
	unsigned long Switch;
  Switch = GPIO_PORTE_DATA_R & 0x01; // read switch
  if (Switch == 0x00){  // if switch not pressed
    LED_ON(); // LED on
	}
  else{
    Toggle_LED(); // toggle LED
	}
}	