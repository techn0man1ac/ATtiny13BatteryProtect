/*
Simple 12V battery LEAD Acid Battery Protection board based ATtiny13 microcontroller 
It's project full open source(PCB and code).
https://github.com/techn0man1ac/ATtiny13BatteryProtect
By Tech01 labs 2023.

Fuses to defalt:
low_fuses=0x6A
high_fuses=0xFF
CPU Frequensy 1,2 MHz

PB0 1 LED 100% Charge level - >12.5 V;
PB1 2 LED 65% Charge level - >11.7 V;
PB2 3 LED 35% Charge level - >11.1 V;
PB3 voltmeter, 50 kOhm and 10 kOhm + 1 kOhm variable resistor;
PB4 Relay.
*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h> 
#include <avr/power.h>
//#include <avr/delay.h>

int main( void )
{
  wdt_reset(); // need to correct work for sleep.h library
  DDRB = 0b10111;  // ports PB0-PB2 and PB4 is outputs, PB3 - input.
  PORTB |= (1 << 2); // blink after wake up

  unsigned int adc_in_main = analogReadOversampled();
  if (adc_in_main <= 410 || adc_in_main >= 472) { // to provide hysteresis after tripping
     // when > 12.0 V or < 13.5 V wake up
    system_sleep();
  }

  for (byte led_test = 0; led_test < 2; led_test++) {  // to do diagnostics
    PORTB = 1 << led_test;
    delay(300);
  }

  while (1) {
    wdt_enable(WDTO_2S); // protection, if not reset after 2 seconds will be reset
    PORTB &= ~(1 << 0);
    PORTB &= ~(1 << 1); // Cut off LEDs 100% 65% 35%
    PORTB &= ~(1 << 2);

    unsigned int adc = analogReadOversampled();

    if (adc >= 358 && adc <= 512) {
      PORTB |= (1 << 4); // if voltage >10.5 V <15 V lets go!
      if (adc >= 445 ) { // >13V  blinking all LEDs
        delay((((542 - adc) / 4) * 50));
      } // flashing intensity depends on battery voltage
      if (adc >= 428 ) {
        PORTB |= (1 << 0);
      }  // LED 1(Green) - 100% charge level >12.5 V.
      if (adc >= 401 ) {
        PORTB |= (1 << 1);
      }  // LED 2(Yellow) - 65% charge level >11.7 V.
      if (adc >= 378 ) {
        PORTB |= (1 << 2);
      }  // LED 3(Red) - 35% charge level >11.1 V.
      if (adc <= 377 ) {
        PORTB |= (1 << 2);  // <11.1 V. blinking red led(charge level < 35%)
        delay((((adc - 350) / 4) * 50)); // flashing intensity depends on battery voltage
        PORTB &= ~(1 << 2);
      }
      wdt_reset(); 
      delay(300);  // 300 ms cycle
    }
    else {
      //PORTB &= ~(1<<4); //
      system_sleep();
    } // If battery voltage >10.5V and <15V - sleep mode
  }
  return 0;
}

unsigned int ADC_READ() {
  ADMUX = 3; // ADC pin
  ADCSRA |= 1 << ADEN;
  ADCSRA |= 1 << ADSC;
  while (!(ADCSRA & (1 << ADIF)));
  ADCSRA |= 1 << ADIF;
  byte low  = ADCL;
  byte high = ADCH;
  ADCSRA &= ~(1 << ADEN);  // отключаем АЦП
  return (high << 8) | low;
}

unsigned int analogReadOversampled() {
  unsigned long aSum = 0;   // the sum of all analog readings
  for (int i = 0; i < 32; i++)
    aSum += ADC_READ(); // read and sum 16 ADС probes
  return aSum >> 5;   // ..
}
void system_sleep() { // Setup sleep mode to 8 seconds
  wdt_reset(); 
  PORTB = 0x00; // All leds is off and relay off
  ADCSRA &= ~(1 << ADEN);  // cut off ADC

  MCUSR &= ~(1 << WDRF);
  /* Start the WDT Config change sequence. */
  WDTCR |= (1 << WDCE) | (1 << WDE);
  /* Configure the prescaler and the WDT for interrupt mode only*/
  WDTCR = (1 << WDP0) | (1 << WDP3) | (1 << WDTIE); // 8sec
  //WDTCR = (1<<WDP2) | (1<<WDP1) | (1<<WDTIE); // 4sec
  //WDTCR = (1<<WDP2) | (1<<WDP0) | (1<<WDTIE); // 0.5sec
  WDTCR |= (1 << WDTIE);

  sei(); // Enable global interrupts

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // full sleep
  while (1) {
    sleep_enable(); 
    sleep_cpu(); // sleeep!
    sleep_disable();
  }
}
