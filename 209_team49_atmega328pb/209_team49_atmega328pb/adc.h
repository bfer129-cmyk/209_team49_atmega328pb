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
#include "config.h"

extern volatile uint16_t v_samples[SAMPLES_PER_CHANNEL];
extern volatile uint16_t i_samples[SAMPLES_PER_CHANNEL];
extern volatile uint8_t samples_ready;
extern volatile uint8_t bias_collection;
extern volatile uint16_t bias_raw;
extern volatile uint16_t freq_counts[CYCLES_TO_AVERAGE];
extern volatile uint8_t  freq_index;
extern volatile uint8_t freq_ready;

void adc_init(void);
void timer1_init(void);
void timer1_start(void);
void timer1_stop(void);
void timer0_init(void);
void timer0_start(void);
void timer0_stop(void);
void zc_init(void);

#endif