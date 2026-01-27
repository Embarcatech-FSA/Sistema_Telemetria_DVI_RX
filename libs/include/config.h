#ifndef CONFIG_H
#define CONFIG_H

// Estrutura de dados compartilhada entre os núcleos
#include <stdint.h>
typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp_read;
} sensor_data_t;

/* VARIÁVEIS USADAS NO CORE 0 */

/* VARIÁVEIS USADAS NO CORE 1 */
#define LED_RED 13
#define LED_BLUE 12
#define BUZZER_PIN 10

// ================= UART CONFIG =================
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_BAUDRATE 115200

#define RX_BUFFER_SIZE 64

#endif // CONFIG_H