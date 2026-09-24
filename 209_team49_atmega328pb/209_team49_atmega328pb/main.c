/*
 * 209_team49_atmega328pb.c
 *
 * Created: 9/09/2026 2:44:27 pm
 * Author : fbrad
 */


#include <avr/io.h>
#include "usart.h"
#include "adc.h"

int main(void) {
    usart_int();
    adc_init();

    // Wait for the bias reading from ADC2 to complete
    while (bias_collection) {
        // wait
    }

    compute_constants();

    usart_transmit_array("Power Monitor Started\r\n");

    while (1) {
        main_processing();

        if (power_result_ready) {
            send_power_data_simple();
            power_result_ready = 0;
        }
    }

    return 0;
}