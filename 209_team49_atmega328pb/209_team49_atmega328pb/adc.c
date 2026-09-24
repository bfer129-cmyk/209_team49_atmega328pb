/*
 * adc.c
 *
 * Created: 9/09/2026 2:45:02 pm
 *  Author: fbrad
 */


#include <avr/io.h>
#include "adc.h"

// ===== GLOBAL BUFFERS =====
volatile uint16_t v_samples[SAMPLES_PER_CHANNEL];
volatile uint16_t i_samples[SAMPLES_PER_CHANNEL];
volatile uint8_t samples_ready = 0;
volatile uint8_t bias_collection = 1;
volatile uint16_t bias_raw = 0;

// ===== INTERNAL STATE =====
static volatile uint8_t sample_index = 0;
static volatile uint8_t adc_channel = 0;
static volatile uint8_t capturing = 0;
static volatile uint8_t first_sample = 1;

// Frequency measurement
volatile uint16_t freq_counts[CYCLES_TO_AVERAGE];   // raw Timer0 counts
volatile uint8_t  freq_index = 0;
volatile uint8_t  freq_ready = 0;
uint8_t timer0_overflow_count = 0;

// ===== TIMER1 INITIALIZATION =====
void timer1_init(void) {
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);   // CTC + prescaler 64
	OCR1A  = OCR1A_VALUE;
	TCNT1  = 0;
	TIMSK1 = (1 << OCIE1A);
}

void timer0_init(void) {
	// Normal mode
	TCCR0A = 0;
	TCCR0B = 0;
	TCCR0B |= (1 << CS01)|(1 << CS00);   // prescaler 64
	
	//enable timer 0 overflow interrupt
	TIMSK0 = (1<<TOIE0);
	
	TCNT0 = 0;
}
// ===== TIMER1 START =====
void timer1_start(void) {
	TCNT1  = 0;
	TIFR1  = (1 << OCF1A);
	TCCR1B |= (1 << CS11) | (1 << CS10);   // prescaler 64
	TIMSK1 |= (1 << OCIE1A);
}

// ===== TIMER1 STOP =====
void timer1_stop(void) {
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
	TIMSK1 &= ~(1 << OCIE1A);
	TIFR1   = (1 << OCF1A);
}

// ===== TIMER0 START =====
void timer0_start(void) {
	TCNT0 = 0;                        // reset counter
	TIFR0 = (1 << TOV0);              // clear any pending overflow flag
	TCCR0B |= (1 << CS01) | (1 << CS00);  // prescaler 64 ? starts clock
}

// ===== TIMER0 STOP =====
void timer0_stop(void) {
	TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));  // stop clock
	TIFR0 = (1 << TOV0);              // clear pending flag
}

// ===== ZC INITIALIZATION (PD2 = INT0) =====
void zc_init(void) {
    DDRD  &= ~(1 << VZC_PIN);
    PORTD |=  (1 << VZC_PIN);

    EICRA |= (1 << ISC01) | (1 << ISC00);   // rising edge
    EIFR  |= (1 << INTF0);
    EIMSK |= (1 << INT0);
}

// ===== ADC INITIALIZATION =====
void adc_init(void) {
    // AREF (external 5V on AREF pin)
    ADMUX &= ~((1 << REFS1) | (1 << REFS0));

    // Start on ADC2 for bias read
    ADMUX &= ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));
    ADMUX |= (1 << MUX1);

    // Prescaler = 64 (250 kHz ADC clock, 52 us conversion)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);

    ADCSRB = 0;
    ADCSRA |= (1 << ADIE);

    timer1_init();
    zc_init();
	timer0_init();
    sei();
}

// ===== ADC INTERRUPT =====
ISR(ADC_vect) {
    if (bias_collection) {
        bias_raw = ADC;
        bias_collection = 0;
        ADMUX &= ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));
        timer1_stop();
        return;
    }

    if (first_sample) {
        first_sample = 0;
        return;
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
        capturing = 0;
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

// ===== INT0 ISR - V-ZC rising edge on PD2 =====
ISR(INT0_vect) {
	// ===== Frequency: read Timer0 =====
	if (!freq_ready)
	{
		freq_counts[freq_index] = TCNT0+256*timer0_overflow_count;
		timer0_overflow_count=0;
		freq_index++;
		if (freq_index >= CYCLES_TO_AVERAGE) {
			freq_index = 0;
			freq_ready = 1;
			timer0_stop();
		}
		TCNT0 = 0;               // reset for next cycle
	}
	
	
    if (bias_collection) return;
	
    if (samples_ready) return;
    if (capturing) return;

    capturing = 1;
    timer1_start();
}

ISR(TIMER0_OVF_vect){
	timer0_overflow_count++;
}