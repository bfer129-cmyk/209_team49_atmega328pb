/*
 * usart.c
 *
 * Created: 9/09/2026 2:47:36 pm
 *  Author: fbrad
 */

#include <avr/io.h>
#include "usart.h"
#include "adc.h"

// ===== STATIC BUFFERS =====
static float v_rms_buf[CYCLES_TO_AVERAGE];
static float i_rms_buf[CYCLES_TO_AVERAGE];
static float p_buf[CYCLES_TO_AVERAGE];
static uint8_t cycle_count = 0;

// ===== POWER DATA STRUCTURE =====
typedef struct {
    float voltage_rms;
    float current_rms;
    float real_power;
    float apparent_power;
    float reactive_power;
    float power_factor;
} PowerData;

static PowerData power_data;

// ===== USART FUNCTIONS =====
void usart_int(uint16_t ubrr) {
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr);
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void usart_transmit_byte(char character) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = character;
}

void usart_transmit_array(char* msg) {
    for (uint8_t i = 0; i < strlen(msg); i++) {
        usart_transmit_byte(msg[i]);
    }
}

// ===== CONVERT ADC TO VOLTAGE =====
void convert_adc_to_voltage(void) {
    for (uint8_t i = 0; i < SAMPLES_PER_CYCLE; i++) {
        float v_meas = (float)v_samples[i] * (ADC_REF / ADC_MAX);
        float i_meas = (float)i_samples[i] * (ADC_REF / ADC_MAX);
        
        v_samples[i] = (uint16_t)((v_meas - V_DC_BIAS) * V_SCALE);
        i_samples[i] = (uint16_t)((i_meas - V_DC_BIAS) * I_SCALE);
    }
}

// ===== CALCULATE RMS AND POWER =====
void calculate_rms_and_power_average(void) {
    float v_rms = 0, i_rms = 0, p_sum = 0;
    
    for (uint8_t i = 0; i < SAMPLES_PER_CYCLE; i++) {
        // Convert stored values back to float
        float v = (float)v_samples[i];
        float i = (float)i_samples[i];
        
        v_rms += v * v;
        i_rms += i * i;
        p_sum += v * i;
    }
    
    v_rms = sqrt(v_rms / SAMPLES_PER_CYCLE);
    i_rms = sqrt(i_rms / SAMPLES_PER_CYCLE);
    float p_avg = p_sum / SAMPLES_PER_CYCLE;
    
    v_rms_buf[cycle_count] = v_rms;
    i_rms_buf[cycle_count] = i_rms;
    p_buf[cycle_count] = p_avg;
    cycle_count++;
}

// ===== AVERAGE AND STORE =====
void average_and_store(void) {
    if (cycle_count < CYCLES_TO_AVERAGE) return;
    
    float v_avg = 0, i_avg = 0, p_avg = 0;
    
    for (uint8_t i = 0; i < CYCLES_TO_AVERAGE; i++) {
        v_avg += v_rms_buf[i];
        i_avg += i_rms_buf[i];
        p_avg += p_buf[i];
    }
    
    v_avg /= CYCLES_TO_AVERAGE;
    i_avg /= CYCLES_TO_AVERAGE;
    p_avg /= CYCLES_TO_AVERAGE;
    
    float s = v_avg * i_avg;
    float q = 0;
    float pf = 0;
    
    if (s > p_avg) {
        q = sqrt(s * s - p_avg * p_avg);
    }
    
    if (s > 0) {
        pf = p_avg / s;
    }
    
    power_data.voltage_rms = v_avg;
    power_data.current_rms = i_avg;
    power_data.real_power = p_avg;
    power_data.apparent_power = s;
    power_data.reactive_power = q;
    power_data.power_factor = pf;
    
    cycle_count = 0;
}

// ===== MAIN PROCESSING =====
void main_processing(void) {
    if (samples_ready) {
        convert_adc_to_voltage();
        calculate_rms_and_power_average();
        average_and_store();
        send_power_data();
        samples_ready = 0;
    }
}

// ===== SEND POWER DATA =====
void send_power_data(void) {
    char buffer[60];
    
    sprintf(buffer, "V:%.2f I:%.3f P:%.2f Q:%.2f S:%.2f PF:%.3f\r\n",
            power_data.voltage_rms,
            power_data.current_rms,
            power_data.real_power,
            power_data.reactive_power,
            power_data.apparent_power,
            power_data.power_factor);
    
    usart_transmit_array(buffer);
}