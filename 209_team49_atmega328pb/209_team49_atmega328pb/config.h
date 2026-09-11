/*
 * confid.h
 *
 * Created: 10/09/2026 1:20:03 pm
 *  Author: fbrad
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

// ===== SYSTEM CLOCK =====
#define F_CPU 2000000UL

// ===== AC SIGNAL =====
#define AC_FREQ 500
#define SAMPLES_PER_CHANNEL 19
#define TOTAL_SAMPLES 38
#define SAMPLE_RATE 19000

// ===== TIMER1 =====
#define OCR1A_VALUE 104

// ===== ADC =====
#define ADC_REF 5.0f
#define ADC_MAX 1023.0f
#define V_DC_BIAS 2.0625f

// ===== USART =====
#define BAUD_RATE 9600
#define BAUD_PRESCALE 12

// ===== AVERAGING =====
#define CYCLES_TO_AVERAGE 10

// ===== COMPONENT VALUES =====
#define Ra 20780.0f
#define Rb 1000.0f
#define R1 3300.0f
#define R2 8200.0f
#define Rs 0.5f

// ===== FIXED-POINT SHIFT =====
// n = 17 chosen so 1023 × (factor × 2^n) < 2^31 for the voltage channel
#define A_SHIFT 17
#define A_SCALE (1UL << A_SHIFT)

#endif