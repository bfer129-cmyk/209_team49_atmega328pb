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

static volatile uint8_t sample_index = 0;
static volatile uint8_t adc_channel = 0;

// ===== TIMER1 INITIALIZATION =====
void timer1_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS10);   // CTC, no prescaler
    OCR1A = OCR1A_VALUE;                    // 104 for 19kHz
    TCNT1 = 0;
    TIMSK1 = (1 << OCIE1A);                 // Enable Compare Match A interrupt
}

// ===== ADC INITIALIZATION =====
void adc_init(void) {
    // AREF (external 5V on AREF pin) ? REFS1=0, REFS0=0
    ADMUX &= ~((1 << REFS1) | (1 << REFS0));

    // Start on ADC0 (voltage)
    ADMUX &= ~((1 << MUX3) | (1 << MUX2) |
               (1 << MUX1) | (1 << MUX0));

    // Prescaler = 8 (250 kHz ADC clock)
    ADCSRA = (1 << ADEN) |
             (0 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);

    // No auto-trigger (software trigger from Timer1 ISR)
    ADCSRB = 0;

    // Enable ADC interrupt
    ADCSRA |= (1 << ADIE);

    timer1_init();   // starts Timer1 + enables compare interrupt
    sei();
}

// ===== ADC INTERRUPT =====
ISR(ADC_vect) {
    if (adc_channel == 0) {
        v_samples[sample_index] = ADC;
        ADMUX |= (1 << MUX0);   // switch to ADC1
        adc_channel = 1;
    } else {
        i_samples[sample_index] = ADC;
        ADMUX &= ~(1 << MUX0);  // switch to ADC0
        adc_channel = 0;

        sample_index++;
        if (sample_index >= SAMPLES_PER_CHANNEL) {
            samples_ready = 1;
            sample_index = 0;
        }
    }
}

// ===== TIMER1 COMPARE MATCH A INTERRUPT =====
ISR(TIMER1_COMPA_vect) {
    ADCSRA |= (1 << ADSC);
}