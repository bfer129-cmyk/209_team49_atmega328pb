/*
 * adc.c
 *
 * Created: 9/09/2026 2:45:02 pm
 *  Author: fbrad
 */


#include <avr/io.h>
#include "adc.h"

volatile uint16_t v_samples[SAMPLES_PER_CHANNEL];
volatile uint16_t i_samples[SAMPLES_PER_CHANNEL];
volatile uint8_t samples_ready = 0;
volatile uint8_t bias_collection = 1;
volatile uint16_t bias_raw = 430;

static volatile uint8_t sample_index = 0;
static volatile uint8_t adc_channel = 0;
volatile uint8_t capturing = 0;
volatile uint8_t first_sample = 1;

// ===== TIMER1 INITIALIZATION (configures but does not start) =====
void timer1_init(void) {
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS10);   // CTC + start clock
	OCR1A  = OCR1A_VALUE;
	TCNT1  = 0;
	TIMSK1 = (1 << OCIE1A);                 // enable compare interrupt
}

// ===== TIMER1 START =====
void timer1_start(void) {
	TCNT1  = 0;                         // reset counter
	TIFR1  = (1 << OCF1A);              // clear any pending compare flag
	TCCR1B |= (1 << CS10);              // set clock source: no prescaler (start)
	TIMSK1 |= (1 << OCIE1A);            // enable compare-match A interrupt
}

// ===== TIMER1 STOP =====
void timer1_stop(void) {
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));  // clear clock source (stop)
	TIMSK1 &= ~(1 << OCIE1A);                               // disable compare interrupt
	TIFR1   = (1 << OCF1A);                                 // clear any pending flag
}



// ===== ZC INITIALIZATION (PC2 = PCINT10, group 1) =====
void zc_init(void) {
	DDRD  &= ~(1 << VZC_PIN);
	PORTD |=  (1 << VZC_PIN);

	EICRA |= (1 << ISC01) | (1 << ISC00);
	EIFR  |= (1 << INTF0);   // ? clear any pending flag
	EIMSK |= (1 << INT0);
}

// ===== ADC INITIALIZATION =====
void adc_init(void) {
    // AREF (external 5V on AREF pin) ? REFS1=0, REFS0=0
    ADMUX &= ~((1 << REFS1) | (1 << REFS0));

    ADMUX &= ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));
    ADMUX |= (1 << MUX1);   // MUX1 = 1 ? ADC2
	
    // Prescaler = 8 (250 kHz ADC clock)
    ADCSRA = (1 << ADEN) |
             (0 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);

    // No auto-trigger (software trigger from Timer1 ISR)
    ADCSRB = 0;

    // Enable ADC interrupt
    ADCSRA |= (1 << ADIE);

    timer1_init();   // starts Timer1
	zc_init(); 
    sei();
}

// ===== ADC INTERRUPT =====
ISR(ADC_vect) {
	if (bias_collection)
	{
		bias_raw = ADC;
		bias_collection = 0;
		ADMUX &= ~((1 << MUX3) | (1 << MUX2) |
		(1 << MUX1) | (1 << MUX0)); //ADC0 Voltage
		timer1_stop();
		return;
	}
	if (first_sample) {
		first_sample = 0;
		return;   // discard first sample
	}
    if (adc_channel == 0) {
        v_samples[sample_index] = ADC;       
    } else {
        i_samples[sample_index] = ADC;
    }

	sample_index++;
	if (sample_index >= SAMPLES_PER_CHANNEL) {
		sample_index = 0;
		timer1_stop();
		capturing = 0;  // ? allow next ZC
		first_sample = 1; 

		if (adc_channel == 0) {
			adc_channel = 1;
			ADMUX |= (1 << MUX0);
			} else {
			adc_channel = 0;
			ADMUX &= ~(1 << MUX0);
			samples_ready = 1;
		}
	}
}

// ===== TIMER1 COMPARE MATCH A INTERRUPT =====
ISR(TIMER1_COMPA_vect) {
    ADCSRA |= (1 << ADSC);
}

// ===== INT0 ISR — V-ZC rising edge on PD2 =====
ISR(INT0_vect) {
	if (bias_collection) return;
	if (samples_ready) return;
	if (capturing) return;  // already capturing
	capturing = 1;
	timer1_start();
}