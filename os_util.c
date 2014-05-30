#include <avr/io.h>
#include <avr/interrupt.h>
#include "os.h"
#include "globals.h"

void start_system_timer() {
   TIMSK0 |= _BV(OCIE0A);  /* IRQ on compare.  */
   TCCR0A |= _BV(WGM01); //clear timer on compare match

   //22KHz settings
   TCCR0B |= _BV(CS01) | _BV(CS01); //slowest prescalar /1024
   OCR0A = 90; 

   //start timer 1 to generate interrupt every 1 second
   OCR1A = 15625;
   TIMSK1 |= _BV(OCIE1A);  /* IRQ on compare.  */
   TCCR1B |= _BV(WGM12) | _BV(CS12) | _BV(CS10); //slowest prescalar /1024
}

void start_audio_pwm() {
   //run timer 2 in fast pwm mode
   TCCR2A |= _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
   TCCR2B |= _BV(CS20);

   DDRD |= _BV(PD3); //make OC2B an output
}

