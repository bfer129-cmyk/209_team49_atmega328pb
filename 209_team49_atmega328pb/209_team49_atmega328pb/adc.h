/*/*
 * adc.h
 *
 * Created: 9/09/2026 2:45:29 pm
 *  Author: fbrad
 */

#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>
#include <avr/interrupt.h>

// ===== CONFIGURATION =====
#define F_CPU 2000000UL
#define SAMPLES_PER_CYCLE 38
#define ADC_REF 5.0
#define ADC_MAX 1023.0
#define V_DC_BIAS 2.1

// Voltage Divider (Ra=20.78k, Rb=1k)
#define V_SCALE ((20.78 + 1.0) / 1.0)  // 21.78

// Current Sensing (R1=3.3k, R2=8.2k, Rs=0.5)
#define I_SCALE (3.3 / (8.2 * 0.5))    // 0.804878

// ===== EXTERN DECLARATIONS =====
extern volatile uint16_t v_samples[SAMPLES_PER_CYCLE];
extern volatile uint16_t i_samples[SAMPLES_PER_CYCLE];
extern volatile uint8_t samples_ready;

// ===== FUNCTION PROTOTYPES =====
void adc_init(void);
void timer1_init(void);

#endif // ADC_H