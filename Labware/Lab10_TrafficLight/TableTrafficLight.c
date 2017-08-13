// ***** 0. Documentation Section *****
// TableTrafficLight.c for Lab 10
// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// east/west car detector connected to PE0 (1=car present)
// north/south car detector connected to PE1 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"
// these is for the systick timer, to help write to the systick registers
#define NVIC_ST_CTRL_R      (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R    (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R   (*((volatile unsigned long *)0xE000E018))

// Label of  our states. These will help to better control
// the finite state machine, than 0,1,2,3,4,5,6,7,8
#define GoW       0             
#define WaitW	    1      
#define	GoS	      2     
#define WaitS	    3      
#define	Walk	    4   
#define	DnWalk1	  5     
#define WalkOff1  6     
#define DnWalk2	  7
#define WalkOff2  8

// ***** 2. Global Declarations Section *****

// Linked data structure
struct Road_State {
  unsigned long Out[2];  // Array of size 2 to hold pattern to car light output and pattern to pedestrian light output
  unsigned long Time; // To hold the delay in 10ms units inbetween state transitions
  unsigned long Next[8]; // Array of size 8 to hold the next state for inputs 0,1,2,3,4,5,6,7,8
}; 

	typedef const struct Road_State Road_State_Typ; // An alias(Road_State_Typ) of a const struct Road_State, to make it shorter to write 
// Array of size 9 of type Road_State_Typ(the const struct type alias), 
// that holds all of the 9 linked data structure of the Finite State Machine	
Road_State_Typ FSM[9]={
	{{0x0C, 0x02},	50,	{GoW,	     GoW,    	 WaitW,    WaitW,	   WaitW,    WaitW,	   WaitW,    WaitW}},
  {{0x14, 0x02},	50,	{GoS,	     WaitW,    GoS,	     GoS,	     Walk,     Walk,	   GoS,	     GoS}},
  {{0x21, 0x02},	50,	{GoS,	     WaitS,    GoS,	     WaitS,	   WaitS,    WaitS,	   WaitS,    WaitS}},
	{{0x22, 0x02},	50,	{GoW,	     GoW,      WaitS,	   GoW,	     Walk,     GoW,	     Walk,	   Walk}},
	{{0x24, 0x08},	50,	{Walk,	   DnWalk1,  DnWalk1,  DnWalk1,  Walk,	   DnWalk1,  DnWalk1,  DnWalk1}},
	{{0x24, 0x02},	50,	{WalkOff1, WalkOff1, WalkOff1, WalkOff1, WalkOff1, WalkOff1, WalkOff1, WalkOff1}},
  {{0x24, 0x00},	50,	{DnWalk2,  DnWalk2,  DnWalk2,  DnWalk2,  DnWalk2,  DnWalk2,	 DnWalk2,  DnWalk2}},
  {{0x24, 0x02},	50,	{WalkOff2, WalkOff2, WalkOff2, WalkOff2, WalkOff2, WalkOff2, WalkOff2, WalkOff2}},
	{{0x24, 0x00},	50,	{GoW,	     GoW,	     GoS,	     GoS,	     Walk,     GoW,      GoS,	     GoW}}
};
  
unsigned long S;  // these will be index to the current state 
unsigned long Input; // and these, variable to hold input from sensor(the switches)

// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void SysTick_Init(void); // Enable Systick timer
void SysTick_Wait(unsigned long delay); // Subroutine to run the Systick Timer
void SysTick_Wait10ms(unsigned long delay); // Subroutine to run the Systick Timer multiple times

// ***** 3. Subroutines Section *****

int main(void){volatile unsigned long delay; 
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210,ScopeOff); // activate grader and set system clock to 80 MHz
  SysTick_Init();   // initialises the systick timer, so that we can create accurate delays between state transitions
  SYSCTL_RCGC2_R  = 0x32;      // 1) initializes clock for PORT B, PORT E and PORT F
  delay = SYSCTL_RCGC2_R;      // 2) no need to unlock
  GPIO_PORTE_AMSEL_R   = 0x00; // 3) disables analog function on pins PE2-0
  GPIO_PORTE_PCTL_R  = 0x00000000; // 4) enables regular GPIO
  GPIO_PORTE_DIR_R  =  0x00;   // 5) makes inputs on pins PE2-0
  GPIO_PORTE_AFSEL_R  = 0x00; // 6)  enables regular function on pins PE2-0
  GPIO_PORTE_DEN_R  = 0x07;    // 7)  enables digital on pins PE2-0
  GPIO_PORTB_AMSEL_R  = 0x00; // 3) disables analog function on pins PB5-0
  GPIO_PORTB_PCTL_R  = 0x00000000; // 4) enables regular GPIO
  GPIO_PORTB_DIR_R  = 0x3F;    // 5) makes outputs on pins PB5-0
  GPIO_PORTB_AFSEL_R   = 0x00; // 6) makes regular function on pins PB5-0
  GPIO_PORTB_DEN_R  = 0x3F;    // 7) enables digital on pins PB5-0
	GPIO_PORTF_AMSEL_R  = 0x00;        // 8) disables analog function on pins PF3,1
  GPIO_PORTF_PCTL_R  = 0x00000000;   // 9)  enables regular GPIO
  GPIO_PORTF_DIR_R  = 0x0A;          // 10) leaves pins PF3,PF1 as outputs   
  GPIO_PORTF_AFSEL_R  = 0x00;        // 11) enables no alternate function      
  GPIO_PORTF_DEN_R  = 0x0A;          // 12) enables digital on pins PF3,PF1
	EnableInterrupts(); // The grader uses interrupts
	
	S = GoW;
  while(1){
		GPIO_PORTB_DATA_R = FSM[S].Out[0];  // sets car traffic lights' in the Data Register
		GPIO_PORTF_DATA_R = FSM[S].Out[1];  // sets pedestrian crossing lights' in the Data Register
    SysTick_Wait10ms(FSM[S].Time);  // calls the Systick timer, to wait 10ms FSM[S].Time times
    Input = GPIO_PORTE_DATA_R;     // reads sensors and stores in Input, to be used for next state transition
    S = FSM[S].Next[Input]; // These will make S the current state after transition from previous state due to Input
  }
}
// Subroutine to initialise Systick Timer control register
// Inputs: None
// Outputs: None
// Notes: .............
void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;               // disable SysTick during setup
  NVIC_ST_CTRL_R = 0x00000005;      // enable SysTick with core clock
}
// Subroutine to run the Systick Timer
// Inputs: one
// Outputs: None
// Notes: The delay parameter is in units of the 80 MHz core clock. (12.5 ns)
void SysTick_Wait(unsigned long delay){
  NVIC_ST_RELOAD_R = delay-1;  // number of counts to wait
  NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){ // wait for count flag
  }
}
// Subroutine to run the Systick Timer multiple times
// Inputs: one
// Outputs: None
// Notes: 800000*12.5ns equals 10ms. Our intended time delay = delay parameter * 10ms 
void SysTick_Wait10ms(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(800000);  // wait 10ms
  }
}
