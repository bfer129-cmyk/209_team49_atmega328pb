/*
 * adc.c
 *
 * Created: 9/09/2026 2:45:02 pm
 *  Author: fbrad
 */

#include <avr/io.h>
#include "adc.h"

#define AC_FREQ 500
#define SAMPLE_RATE (AC_FREQ * SAMPLES_PER_CYCLE)  // 19,000 Hz

// ===== GLOBAL VARIABLES =====
volatile uint16_t v_samples[SAMPLES_PER_CYCLE];
volatile uint16_t i_samples[SAMPLES_PER_CYCLE];
volatile uint8_t samples_ready = 0;
static volatile uint8_t sample_index = 0;
static volatile uint8_t adc_channel = 0;

// ===== TIMER1 INITIALIZATION =====
void timer1_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS10);  // CTC, no prescaler
    OCR1A = (F_CPU / SAMPLE_RATE) - 1;    // 104
}

// ===== ADC INITIALIZATION =====
void adc_init(void) {
    // AVCC reference, ADC0 (voltage channel)
    ADMUX = (1 << REFS0) | (0 << MUX3) | (0 << MUX2) | (0 << MUX1) | (0 << MUX0);
    
    // Prescaler = 8 (250kHz ADC clock)
    ADCSRA = (1 << ADEN) |
             (0 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);
    
    // Timer1 Compare Match A triggers ADC
    ADCSRB = (1 << ADTS2) | (0 << ADTS1) | (0 << ADTS0);
    
    // Enable ADC interrupt
    ADCSRA |= (1 << ADIE);
    
    // Start Timer1
    TCNT1 = 0;
    
    // Enable global interrupts
    sei();
}

// ===== ADC INTERRUPT =====
ISR(ADC_vect) {
    if (adc_channel == 0) {
        // Store voltage sample
        v_samples[sample_index] = ADC;
        
        // Switch to current channel (ADC1)
        ADMUX = (ADMUX & 0xF8) | 1;
        adc_channel = 1;
        
    } else {
        // Store current sample
        i_samples[sample_index] = ADC;
        
        // Switch back to voltage channel (ADC0)
        ADMUX = (ADMUX & 0xF8) | 0;
        adc_channel = 0;
        
        sample_index++;
        
        if (sample_index >= SAMPLES_PER_CYCLE) {
            samples_ready = 1;
            sample_index = 0;
        }
    }
}