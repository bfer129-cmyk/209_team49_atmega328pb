/*/*
 * usart.h
 *
 * Created: 9/09/2026 2:47:57 pm
 *  Author: fbrad
 */

#ifndef USART_H
#define USART_H

#include <avr/io.h>
#include <math.h>
#include <string.h>

#define CYCLES_TO_AVERAGE 10

// ===== FUNCTION PROTOTYPES =====
void usart_int(uint16_t ubrr);
void usart_transmit_byte(char character);
void usart_transmit_array(char* msg);

// Power processing functions
void main_processing(void);
void send_power_data(void);

#endif // USART_H