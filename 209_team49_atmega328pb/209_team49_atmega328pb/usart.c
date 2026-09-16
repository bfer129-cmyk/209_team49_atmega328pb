/*
 * usart.c
 *
 * Integer-math power processing with runtime-computed constants.
 *
 * Units:
 *   Voltage: cV   (centivolts,  1 cV = 0.01 V)
 *   Current: mA   (milliamps,   1 mA = 0.001 A)
 *   Power:   cW   (centiwatts,  1 cW = 0.01 W)
 */

#include <avr/io.h>
#include "usart.h"
#include "adc.h"
#include "config.h"

// ===== RUNTIME-COMPUTED CONSTANTS =====
static int32_t A_V;
static int32_t A_I;
static int32_t V_OFFSET;
static int32_t I_OFFSET;

// ===== ACCUMULATORS =====
static int32_t  v_sq_sum    = 0;
static int32_t  i_sq_sum    = 0;
static int32_t  p_sum       = 0;
static uint16_t v_peak_max  = 0;
static uint16_t i_peak_max  = 0;
static uint16_t cycle_count = 0;

// ===== POWER DATA =====
PowerData power_data;
volatile uint8_t power_result_ready = 0;

// ===== COMPUTE CONSTANTS =====
void compute_constants(void) {
    float V_DC_BIAS = ((float)bias_raw * ADC_REF) / ADC_MAX;

    // -- Voltage --
    float factor_V = (ADC_REF / ADC_MAX) * ((Ra + Rb) / Rb) * 100.0f;
    A_V = (int32_t)(factor_V * A_SCALE + 0.5f);

    float v_off = ((Ra + Rb) / Rb) * 100.0f * V_DC_BIAS;
    V_OFFSET = (int32_t)(v_off * A_SCALE + 0.5f);

    // -- Current --
    float factor_I = (ADC_REF / ADC_MAX) * (R1 / (R2 * Rs)) * 1000.0f;
    A_I = (int32_t)(factor_I * A_SCALE + 0.5f);

    float i_off = (R1 / (R2 * Rs)) * 1000.0f * V_DC_BIAS;
    I_OFFSET = (int32_t)(i_off * A_SCALE + 0.5f);
}

// ===== USART =====
void usart_int(void) {
    uint16_t ubrr = (uint16_t)BAUD_PRESCALE;
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

// ===== INTEGER SQUARE ROOT =====
static uint16_t isqrt32(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n;
    uint32_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return (uint16_t)x;
}

// ===== CONVERT + ACCUMULATE =====
void process_adc_samples(void) {
    uint16_t v_peak_cycle = 0;
    uint16_t i_peak_cycle = 0;

    for (uint8_t i = 0; i < SAMPLES_PER_CHANNEL; i++) {
        int16_t v_cV = (int16_t)(((int32_t)v_samples[i] * A_V - V_OFFSET) >> A_SHIFT);
        int16_t i_mA = (int16_t)(((int32_t)i_samples[i] * A_I - I_OFFSET) >> A_SHIFT);

        uint16_t av = (v_cV >= 0) ? (uint16_t)v_cV : (uint16_t)(-v_cV);
        uint16_t ai = (i_mA >= 0) ? (uint16_t)i_mA : (uint16_t)(-i_mA);
        if (av > v_peak_cycle) v_peak_cycle = av;
        if (ai > i_peak_cycle) i_peak_cycle = ai;

        v_sq_sum += (int32_t)v_cV * v_cV;
        i_sq_sum += (int32_t)i_mA * i_mA;
        p_sum    += (int32_t)v_cV * i_mA;
    }

    if (v_peak_cycle > v_peak_max) v_peak_max = v_peak_cycle;
    if (i_peak_cycle > i_peak_max) i_peak_max = i_peak_cycle;

    cycle_count++;
}

// ===== AVERAGE AND STORE =====
void average_and_store(void) {
    if (cycle_count < CYCLES_TO_AVERAGE) return;

    uint16_t n = (uint16_t)(SAMPLES_PER_CHANNEL * CYCLES_TO_AVERAGE);

    uint16_t v_rms_cV = isqrt32((uint32_t)v_sq_sum / n);
    uint16_t i_rms_mA = isqrt32((uint32_t)i_sq_sum / n);
    uint16_t p_cW     = (uint16_t)(((uint32_t)p_sum / n) / 1000);

    power_data.voltage_rms_cV  = v_rms_cV;
    power_data.voltage_peak_cV = v_peak_max;
    power_data.current_rms_mA  = i_rms_mA;
    power_data.current_peak_mA = i_peak_max;
    power_data.real_power_cW   = p_cW;

    v_sq_sum = 0;
    i_sq_sum = 0;
    p_sum = 0;
    v_peak_max = 0;
    i_peak_max = 0;
    cycle_count = 0;

    power_result_ready = 1;
}

// ===== MAIN PROCESSING =====
void main_processing(void) {
    if (samples_ready) {
        process_adc_samples();
        samples_ready = 0;
        average_and_store();
    }
}

// ===== SEND POWER DATA =====
void send_power_data_simple(void) {
    usart_transmit_voltage(power_data.voltage_rms_cV,  "rV");
    usart_transmit_voltage(power_data.voltage_peak_cV, "pV");
    usart_transmit_current(power_data.current_rms_mA,  "rC");
    usart_transmit_current(power_data.current_peak_mA, "pC");
    usart_transmit_power  (power_data.real_power_cW,   "rP");
    usart_transmit_array("\r\n");
}

// ===== TRANSMIT: XX.XXV =====
void usart_transmit_voltage(uint16_t voltage_cV, char* label) {
    usart_transmit_array(label);
    usart_transmit_array(": ");

    uint16_t tens = (voltage_cV / 1000) % 10;
    if (tens != 0) usart_transmit_byte('0' + tens);

    usart_transmit_byte('0' + (voltage_cV / 100) % 10);
    usart_transmit_byte('.');
    usart_transmit_byte('0' + (voltage_cV / 10) % 10);
    usart_transmit_byte('0' + voltage_cV % 10);
    usart_transmit_byte('V');
    usart_transmit_array("\r\n");
}

// ===== TRANSMIT: XXXmA or X,XXXmA =====
void usart_transmit_current(uint16_t current_mA, char* label) {
    usart_transmit_array(label);
    usart_transmit_array(": ");

    if (current_mA >= 1000) {
        usart_transmit_byte('0' + (current_mA / 1000) % 10);
        usart_transmit_byte(',');
    }

    usart_transmit_byte('0' + (current_mA / 100) % 10);
    usart_transmit_byte('0' + (current_mA / 10) % 10);
    usart_transmit_byte('0' + current_mA % 10);
    usart_transmit_array("mA\r\n");
}

// ===== TRANSMIT: XX.XXW =====
void usart_transmit_power(uint16_t power_cW, char* label) {
    usart_transmit_array(label);
    usart_transmit_array(": ");

    uint16_t tens = (power_cW / 1000) % 10;
    if (tens != 0) usart_transmit_byte('0' + tens);
    usart_transmit_byte('0' + (power_cW / 100) % 10);
    usart_transmit_byte('.');
    usart_transmit_byte('0' + (power_cW / 10) % 10);
    usart_transmit_byte('0' + power_cW % 10);
    usart_transmit_byte('W');
    usart_transmit_array("\r\n");
}