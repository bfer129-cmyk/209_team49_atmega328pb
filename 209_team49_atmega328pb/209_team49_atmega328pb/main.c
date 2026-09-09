/*
 * 209_team49_atmega328pb.c
 *
 * Created: 9/09/2026 2:44:27 pm
 * Author : fbrad
 */

#define F_CPU 2000000UL

#include <avr/io.h>
#include "usart.h"
#include "adc.h"

#define BAUD_RATE 9600
#define BAUD_PRESCALE (((F_CPU / (BAUD_RATE * 16UL))) - 1)

int main(void) {
    uint16_t ubrr_rate = (uint16_t)BAUD_PRESCALE;
    
    usart_int(ubrr_rate);
    adc_init();
    timer1_init();
    
    usart_transmit_array("Power Monitor Started\r\n");
    
    while (1) {
        main_processing();
    }
    
    return 0;
}