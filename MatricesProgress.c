//Scheduler Code
#include <avr/io.h>
//#include "io.c" //included from directory

void ADC_init() {
	/*ADMUX = 1;*/
	
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADENl: setting this bit enables analog to digital conversion
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto triggering.
	// Since we are in Free Running Mode, a new conversion
	// will trigger whenever the previous conversion completes
	
	//
}
//------------------------------Timer Directory Start-------------------
// Permission to copy is granted provided that this header remains intact. 
// This software is provided with no warranties.

////////////////////////////////////////////////////////////////////////////////

#ifndef TIMER_H
#define TIMER_H

#include <avr/interrupt.h>

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1ms
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B 	= 0x0B;	// bit3 = 1: CTC mode (clear timer on compare)
					// bit2bit1bit0=011: prescaler /64
					// 00001011: 0x0B
					// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
					// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A 	= 125;	// Timer interrupt will be generated when TCNT1==OCR1A
					// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
					// So when TCNT1 register equals 125,
					// 1 ms has passed. Thus, we compare to 125.
					// AVR timer interrupt mask register

	TIMSK1 	= 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1 = 0;

	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;

	//Enable global interrupts
	SREG |= 0x80;	// 0x80: 1000000
}

void TimerOff() {
	TCCR1B 	= 0x00; // bit3bit2bit1bit0=0000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect)
{
	// CPU automatically calls when TCNT0 == OCR0 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; 			// Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { 	// results in a more efficient compare
		TimerISR(); 				// Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

#endif //TIMER_H
//-------------------End Timer Directory--------------------------------
// ------Task Scheduler Data Structure ---------------------------------
/* Struct for Tasks represent a running process in our simple real-time
			operating system. */

typedef struct _task {
	/*Tasks should have members that include state, period,
		a measurement of elapsed time, and a function pointer.*/
		
		signed char state; //Task's current state
		unsigned long int period; //Task period
		unsigned long int elapsedTime; //Time elapsed since last tick
		int (*TickFct)(int);
} task;

// ------End Task Scheduler Data structure -----------------------------

// ------Shared Variables ----------------------------------------------
static unsigned char column_val = 0x01; //val: 0x01 sel: 0x01 = top left corner 
static unsigned char column_sel = 0x01; 
//val: 0x01 sel: 0x01 = top left corner 
//move val << to move right 
//move sel << to go up and down
//unsigned short ADCINPUT = 0x0000;
// ------End Shared Variables-------------------------------------------
//---------------GetJoystickInput---------------------------------------
// enum JoystickInput_states{left,center, right};
// int JoystickInputSM(int state){
// 	ADCINPUT = ADC;
// 	switch(state)// transitions
// 	{
// 		case center: break;
// 		
// 		case left:break;
// 		
// 		case right: break;
// 		default: state = center;
// 		break;
// 	}
// 	
// 	switch(state)
// 	
// };

//---------------EndJoystickInput---------------------------------------
//-------------MatrixdisplaySM------------------------------------------
enum Matrixdisplay_states{Matrixdisplay1};
int MatrixdisplaySM(int state){
	
	switch (state)// transitions
	{
		case Matrixdisplay1:
			break;
		default: state = Matrixdisplay1;
			break;
	}	
	
	switch (state)// Actions
	{
	
		case Matrixdisplay1: // If illuminated LED in bottom right corner
			break;
		default: break;
	}
}
//-------------ENDMatrixdisplaySM---------------------------------------


//------------- Display SM ---------------------------------------------

enum display_states {display_display};

int displaySMTick(int state){
	//Local Variables
	
	
	switch (state) { //transitions
		case display_display: state = display_display; break;
		default: state = display_display; break;
	}
	switch (state) {
		case display_display:
			break;
	}
	 PORTB = column_sel; // PORTA displays column pattern
	 PORTC = ~column_val; // PORTB selects column to display pattern
	return state;
}
//---------------End DisplaySM---------------------------------------

int main(){
	DDRA = 0x0F; PORTA = 0xFF;//joystick
	DDRB = 0xFF; PORTB = 0x00;//Columns
	DDRC = 0xFF; PORTC = 0xFF;//Blue Rows
	DDRD = 0xFF; PORTD = 0xFF;//Green Rows
	ADC_init();
	unsigned short ADCINPUT = 0x0000;
	// Declare an array of tasks
	static task task1, task2;
	task *tasks[] = {&task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
//	int iter = 0;
	unsigned long GCD = 10;
// 	for (iter = 1; iter < numTasks; iter++)
// 	{
// 		GCD = findGCD(GCD, tasks[iter]->period);
// 	}
// 	
	// Task 1 (pauseButtonToggleSM)
	task1.state = Matrixdisplay1; //Task initial state.
	task1.period = 100; //Task Period
	task1. elapsedTime = task1.period; //Task current elapsed time.
	task1.TickFct = &MatrixdisplaySM; //Function pointer for the tick.
	
	// Task 4 (displaySM)
	task2.state = display_display; //Task initial state.
	task2.period = 100; //Task Period
	task2. elapsedTime = task2.period; //Task current elapsed time.
	task2.TickFct = &displaySMTick; //Function pointer for the tick.
	
	// Set the timer and turn it on
	
	TimerSet(GCD);
	TimerOn();
	
	unsigned short i;// scheduler for loop iterator
	
	while(1){
		ADCINPUT = ADC;
		// 		tempB = (char)ADCINPUT;
		// 		tempD = (char)(ADCINPUT >> 8);
		if(ADCINPUT < 341)
		{
			PORTA = 0x20;
		} //left
		if(ADCINPUT > 682)
		{
			PORTA = 0x10;
		} // right
		if ( ADCINPUT < 682 && ADCINPUT > 341)
		{
			PORTA = 0x00;
		} // rest
		for(i = 0; i < numTasks; i++)
		{
			if ( tasks[i]->elapsedTime == tasks[i]->period)
			{
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state); //Set next State
				tasks[i]->elapsedTime = 0; //reset the elapsed time for the next tick
			}
			tasks[i]-> elapsedTime+= GCD;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	return 0; //Error: Program should not exit
}
