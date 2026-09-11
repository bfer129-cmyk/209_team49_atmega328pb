/*/*
 * usart.h
 *
 * Created: 9/09/2026 2:47:57 pm
 *  Author: fbrad
 */

#ifndef USART_H
#define USART_H

#include <avr/io.h>
#include <string.h>
#include "config.h"

// ===== POWER DATA STRUCTURE =====
typedef struct {
	uint16_t voltage_rms_cV;
	uint16_t voltage_peak_cV;
	uint16_t current_rms_mA;
	uint16_t current_peak_mA;
	uint16_t real_power_cW;
} PowerData;

extern PowerData power_data;
extern volatile uint8_t power_result_ready;

void compute_constants(void);

void usart_int(void);
void usart_transmit_byte(char character);
void usart_transmit_array(char* msg);

void usart_transmit_voltage(uint16_t voltage_cV, char* label);
void usart_transmit_current(uint16_t current_mA, char* label);
void usart_transmit_power(uint16_t power_cW, char* label);

void main_processing(void);
void process_adc_samples(void);
void average_and_store(void);
void send_power_data(void);
void send_power_data_simple(void);

#endif