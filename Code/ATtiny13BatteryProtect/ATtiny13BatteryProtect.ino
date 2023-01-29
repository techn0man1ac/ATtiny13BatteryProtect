/*
  PB3 вольтметр, делитель напряжения, 50 кОм и 10 кОм + 1 кОм подстроечный(по плюсу)
  PB4 реле
  PB0 1-й светодиод 100% заряда - 12.5 В.
  PB1 2-й светодиод 65% заряда - 11.7 В.
  PB2 3-й светодиод 35% заряда - 11.1 В.

  Фюзы по умолчанию:
  low_fuses=0x6A
  high_fuses=0xFF
  Частота 1,2 MHz
*/

#include <avr/io.h>
#include <avr/wdt.h> // здесь организована работа с ватчдогом
#include <avr/sleep.h> // здесь описаны режимы сна
#include <avr/power.h>
//#include <avr/delay.h>

int main( void )
{
  wdt_reset(); // надо же, сколько над этим бился
  // просто ватчдог после перезагрузки включен
  DDRB = 0b10111;  // порти PB0-PB2 и PB4 встановленні на вихід, PB3 на вхід.
  PORTB |= (1 << 2); // подмигнём, типа спим но питание есть

  /*for(byte led_test = 1; led_test >= 0; led_test--){    // проведемо діагностику
    PORTB = 1<<led_test;
    delay(300); //_delay_ms(300);
    }*/

  unsigned int adc_in_main = analogReadOversampled();
  if (adc_in_main <= 410 || adc_in_main >= 472) { // для обеспечения гистерезиса после отключения
    // при > 12.0 В или < 13.5 В просыпаемся
    system_sleep();
  }

  for (byte led_test = 0; led_test < 2; led_test++) {  // проведемо діагностику
    PORTB = 1 << led_test;
    delay(300);
  }

  while (1) {
    wdt_enable(WDTO_2S); // защита от случайных зависаний
    // если не сбросить через 2 сек будет ресет
    PORTB &= ~(1 << 0);
    PORTB &= ~(1 << 1); // тушым светодиоди 100% 65% 35%
    PORTB &= ~(1 << 2);

    unsigned int adc = analogReadOversampled();

    if (adc >= 358 && adc <= 512) {
      PORTB |= (1 << 4); // напряжение > 10.5 В < 15 В поехали!
      if (adc >= 445 ) { // > 13 В мигаем всема светодиодами
        delay((((542 - adc) / 4) * 50));
      } // интенсивность мигания зависит от напряжения
      if (adc >= 428 ) {
        PORTB |= (1 << 0);
      }  // 1-й светодиод 100% заряда > 12.5 В.
      if (adc >= 401 ) {
        PORTB |= (1 << 1);
      }  // 2-й светодиод 65% заряда > 11.7 В.
      if (adc >= 378 ) {
        PORTB |= (1 << 2);
      }  // 3-й светодиод 35% заряда > 11.1 В.
      if (adc <= 377 ) {
        PORTB |= (1 << 2);  // < 11.1 В. мигаем светодиодом 30%
        delay((((adc - 350) / 4) * 50)); // интенсивность мигания зависит от напряжения
        PORTB &= ~(1 << 2);
      }
      wdt_reset(); // сбросим защиту от зависаний
      delay(300);  // ждём как минимум 0.3 сек
    }
    else {
      //PORTB &= ~(1<<4); //
      system_sleep();
    } // если условие > 10.5 В < 15 В не выполняется - сон
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
void system_sleep() {
  wdt_reset(); // сбрасываем ватчдог measure current
  PORTB = 0x00; // потушым все диоди и откл. реле
  ADCSRA &= ~(1 << ADEN);  // вимикаємо АЦП

  MCUSR &= ~(1 << WDRF);
  /* Start the WDT Config change sequence. */
  WDTCR |= (1 << WDCE) | (1 << WDE);
  /* Configure the prescaler and the WDT for interrupt mode only*/
  WDTCR = (1 << WDP0) | (1 << WDP3) | (1 << WDTIE); // 8sec
  //WDTCR = (1<<WDP2) | (1<<WDP1) | (1<<WDTIE); // 4sec
  //WDTCR = (1<<WDP2) | (1<<WDP0) | (1<<WDTIE); // 0.5sec
  WDTCR |= (1 << WDTIE);

  sei(); // Enable global interrupts

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // если спать - то на полную
  while (1) {
    sleep_enable(); // разрешаем сон
    sleep_cpu(); // спать!
    sleep_disable();
  }
}
