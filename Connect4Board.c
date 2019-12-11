/*
 * Lab07.c
 *
 * Created: 10/26/2019 1:35:01 PM
 *  Author: klabo
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
//---------ADC AND Timer----------------------------------------------
void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADENl: setting this bit enables analog to digital conversion
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto triggering.
	// Since we are in Free Running Mode, a new conversion 
	// will trigger whenever the previous conversion completes
	
	//
}
volatile unsigned char TimerFlag = 0; //TimerISR() sets this to a 1. C programmer should clear to 0.
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; //Start count from here, down to 0. Default to 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1 ms ticks

void TimerOn()
{
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; //bit3 = 0: CTC mode (clear timer on compare)
	//bit2bit1bit0 = 011: pre-scaler /64
	// 00001011: 0x0B
	// so, 8MHz clock or 8,000,000 /64 =125,000 ticks/s
	// Thus, TCNT1 register will count as 125,000 ticks/s
	//AVR output compare register OCR1A.
	OCR1A = 125;   // Timer interrupt will be generated when TCNT1 == OCR1A
	// We want a 1 ms tick. 0.001s *125,000 ticks/s = 125
	// so when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt
	
	// Initialize avr counter
	TCNT1=0;
	
	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	
	//Enable global interrupts
	SREG |=0x80; // 0x80: 1000000
	
}

void TimerOff()
{
	TCCR1B = 0x00; // bit3bitbit0 -000: Timer off
}

void TimerISR()
{
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect)
{
	//CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) //results in a more efficient compare
	{
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

//Set TimerISR() to tick every M ms
void TimerSet(unsigned long M)
{
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
//-------END ADC AND TIMER--------------------------------------------

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
	unsigned char REDpattern[]={0,0,0,0,0,0,0,0};
	unsigned char GREENpattern[]={0,0,0,0,0,0,0,0};
	unsigned char availablespot[]={1,1,1,1,1,1,1,1};
	unsigned int arrayposition = 0;
	unsigned int currplayer[] = {0,0,0} ; //0forRed 1forgreen [color, column, row]
	unsigned char pcolor = 0x00;
	unsigned int pcolumn = 0x00;
	unsigned int plevel = 0x01;
//--------End Shared Variables----------------------------------------
//----------------joystick selection---------------------------------
enum joystick_states {left, right, wait, select };
int joystickTick(int state){
	//Local Variables
	unsigned short ADCINPUT;
	ADCINPUT = ADC;
	unsigned char selectbutton = (~PINA & 0x04);
	
	switch (state) { //transitions
		case wait: //if left
                    if (ADCINPUT < 500)
                    {
						state = left;
						break;
					}
					//if right
					else if (ADCINPUT > 682)
					{
						state = right;
						break;
					}
					//if select
					else if (selectbutton)
					{
						state = select;
						break;
					}
					//if wait
						break;
		case left: //wait 
					state = wait;
					break;
		case right: //wait
					state = wait;
					break;
		case select: //wait
					state = wait;
					break;
					 
		default: state = wait; break;
	}
	switch (state) {//Actions
		
		case wait:
				break;
		case left:
				
				if (pcolumn == 7)
				{
					pcolumn = 7;
				}
				else
				{
					pcolumn += 1;
					while(plevel == 128)
					{
						pcolumn += 1;
					}
				}
				plevel = availablespot[pcolumn];
				break;
				
		case right: 
				
				if (pcolumn == 0)//column
				{
					pcolumn = 0;
				}
				else
				{
					pcolumn -= 1;
					while(plevel == 128)
					{
						pcolumn -= 1;
					}
				
				}
				
				plevel = availablespot[pcolumn];
				break;
				
		case select: 

				//updates red players array
				if(pcolor == 0)
				{
					REDpattern[pcolumn] = REDpattern[pcolumn]  | availablespot[pcolumn];
				}
				//updates Green players array
				else if(pcolor == 1)
				{
					GREENpattern[pcolumn] = GREENpattern[pcolumn]  | availablespot[pcolumn];				
				}
				//updates available spot array
				availablespot[pcolumn] = availablespot[pcolumn] + availablespot[pcolumn];
				
				//Flips player
				pcolor = (~pcolor & 0x01);
				plevel = availablespot[pcolumn];
				break;
		default: break;
	}
	return state;
	
}

//---------------End Joystick Selection-----------------------------
//------------- Display SM ---------------------------------------------

enum display_states {display_displaygreen, display_displayred};
	void transmit_dataGreen (unsigned char data) {
		int i;
		for (i = 0; i < 8 ; ++i) {
			// Sets SRCLR to 1 allowing data to be set
			// Also clears SRCLK in preparation of sending data
			PORTB = 0x08;
			// set SER = next bit of data to be sent.
			PORTB |= ((data >> i) & 0x01);
			// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
			PORTB |= 0x02;
		}
		// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
		PORTB |= 0x04;
		// clears all lines in preparation of a new transmission
		PORTB = 0x00;
	}
	void transmit_dataRed(unsigned char data) {
		int i;
		for (i = 0; i < 8 ; ++i) {
			// Sets SRCLR to 1 allowing data to be set
			// Also clears SRCLK in preparation of sending data
			PORTC = 0x08;
			// set SER = next bit of data to be sent.
			PORTC |= ((data >> i) & 0x01);
			// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
			PORTC |= 0x02;
		}
		// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
		PORTC |= 0x04;
		// clears all lines in preparation of a new transmission
		PORTC = 0x00;
	}
int displaySMTick(int state){
	//Local Variables
	static unsigned char column_sel = 0x7F; // grounds column to display pattern
	unsigned char red;
	unsigned char green;
	unsigned char clear = 0xFF;
	
	switch (state) { //transitions
		case display_displaygreen: state = display_displayred; break;
		case display_displayred: state = display_displaygreen; break; 
		default: state = display_displayred; break;
	}
	switch (state) {//Actions
		
		case display_displayred:
		transmit_dataGreen(clear);
			
// 		if(arrayposition < 7)
// 		{
// 			arrayposition += 1;
// 		}
// 		else 
// 		{
// 			arrayposition = 0;
// 		}
						
			red = REDpattern[arrayposition];
			if (pcolor == 0 && arrayposition == pcolumn)
		{
			red =  red | plevel;
		}
		transmit_dataRed(~red);
		break;
		
		case display_displaygreen:
		transmit_dataRed(clear);
		// If illuminated LED in bottom right corner
		if (column_sel == 0xFE )
		{
			column_sel = 0x7F; // display far left column
		}
		else
		{
			column_sel = (column_sel >> 1) | 0x80;
		}
		
		if(arrayposition < 7)
		{
			arrayposition += 1;
		}
		else
		{
			arrayposition = 0;
		}
		
		green = GREENpattern[arrayposition];
		
		if (pcolor == 1 && arrayposition == pcolumn)
		{
			green = green | availablespot[pcolumn];
		}
		transmit_dataGreen(~green);
		break;
		default: break;
	}




PORTD = ~column_sel; // PORTB selects column to display pattern
return state;
		
}
//---------------End DisplaySM---------------------------------------

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;//joystick & buttons
	DDRB = 0xFF; PORTB = 0x00;//Green
	DDRC = 0xFF; PORTC = 0x00;//Red
	DDRD = 0xFF; PORTD = 0x00;//Rows or Columns
	TimerSet(1);
	TimerOn();
	ADC_init();
	// Declare an array of tasks
	static task task1, task2;
	task *tasks[] = {&task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	unsigned long GCD = 1;
		
	while (1)
	{

	
	// Task 1 (displaySM)
	task1.state = display_displayred; //Task initial state.
	task1.period = 1; //Task Period
	task1. elapsedTime = task1.period; //Task current elapsed time.
	task1.TickFct = &displaySMTick; //Function pointer for the tick.
	
	
		//Task 2 (joystickSM)
		task2.state = wait; //Task initial state.
		task2.period = 100; //Task Period
		task2. elapsedTime = task2.period; //Task current elapsed time.
		task2.TickFct = &joystickTick; //Function pointer for the tick.
		// Set the timer and turn it on
		
	TimerSet(GCD);
	TimerOn();
	unsigned short i;// scheduler for loop iterator

	while(1)
	{
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
}
