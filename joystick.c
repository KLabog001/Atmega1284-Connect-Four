/*
 * Lab07.c
 *
 * Created: 10/26/2019 1:35:01 PM
 *  Author: klabo
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
volatile unsigned char TimerFlag = 0; //TimerISR() sets this to a 1. C programmer should clear to 0.
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; //Start count from here, down to 0. Default to 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1 ms ticks

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADENl: setting this bit enables analog to digital conversion
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto triggering.
	// Since we are in Free Running Mode, a new conversion 
	// will trigger whenever the previous conversion completes
	
	//
}

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


int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00; 
// 	LCD_init();
// 	TimerSet(200);
// 	TimerOn();
	unsigned short ADCINPUT = 0x0000;
	//  unsigned short MAX = 736;
	//unsigned short max = 176
	ADC_init();

	while (1)
	{
		ADCINPUT = ADC;
// 		tempB = (char)ADCINPUT;
// 		tempD = (char)(ADCINPUT >> 8);
		if(ADCINPUT < 341)
		{
			PORTB = 0x02;	
		} //left
		if(ADCINPUT > 682)
		{
			PORTB = 0x01;
		} // right
		if ( ADCINPUT < 682 && ADCINPUT > 341)
		{
			PORTB = 0x00;
		} // rest
		
		
	}
	
	return 0;
}
