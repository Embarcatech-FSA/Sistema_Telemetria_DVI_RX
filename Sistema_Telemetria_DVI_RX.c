/***
 * Core 0 - Recebe dados via UART.
 * Core 1 - Exibe no display via DVI.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "pico/multicore.h"
#include "pico/bootrom.h" 
#include "pico/sem.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/structs/ssi.h"
#include "hardware/dma.h"
#include "hardware/adc.h" 
#include "dvi.h"
#include "dvi_serialiser.h"
#include "common_dvi_pin_configs.h"
#include "tmds_encode_font_2bpp.h"

// Inclusão do arquivo de fonte
#include "./assets/font_teste.h"
#define FONT_N_CHARS 95
#define FONT_FIRST_ASCII 32

// AJUSTES PRINCIPAIS PARA DUPLICAÇÃO 3X (8x24)
#define FONT_CHAR_WIDTH 8     
#define FONT_CHAR_HEIGHT 24         
#define FONT_ORIGINAL_HEIGHT 8      
#define FONT_SCALE_FACTOR (FONT_CHAR_HEIGHT / FONT_ORIGINAL_HEIGHT) // Fator 3

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define VREG_VSEL VREG_VOLTAGE_1_20
#define DVI_TIMING dvi_timing_640x480p_60hz

struct dvi_inst dvi0;

// Definições do terminal de caracteres
// CHAR_COLS: 640 / 8 = 80 
// CHAR_ROWS: 480 / 24 = 20 
#define CHAR_COLS (FRAME_WIDTH / FONT_CHAR_WIDTH)
#define CHAR_ROWS (FRAME_HEIGHT / FONT_CHAR_HEIGHT) 

// Buffers para caracteres e cores
#define COLOUR_PLANE_SIZE_WORDS (CHAR_ROWS * CHAR_COLS * 4 / 32)
// #define COLOUR_PLANE_SIZE_WORDS ((CHAR_ROWS * CHAR_COLS) / 8)

char charbuf[CHAR_ROWS * CHAR_COLS];
uint32_t colourbuf[3 * COLOUR_PLANE_SIZE_WORDS];

// Definições para o histórico (Mínimos e Máximos)
static float max_temp = -999.0f;
static float min_temp = 999.0f;
static float max_hum  = -999.0f;
static float min_hum  = 999.0f;
volatile uint32_t core1_heartbeat = 0;

// Uso do botão B para o BOOTSEL
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

#define USE_MOCK_DATA 1

// declaração de funções
void core1_main();
void draw_border();
static inline void set_colour(uint x, uint y, uint8_t fg, uint8_t bg);
static inline void set_char(uint x, uint y, char c);

// --- FUNÇÕES DE DASHBOARD ---

void draw_dashboard(float current_temp, float current_hum) {
    // 1. Limpa área interna (preserva bordas)
    for (int y = 1; y < CHAR_ROWS - 1; y++) {
        for (int x = 1; x < CHAR_COLS - 1; x++) {
            set_char(x, y, ' ');
            set_colour(x, y, 0x00, 0x00);
        }
    }

    // 2. Título Centralizado
    const char *title = "MONITORAMENTO AMBIENTAL";
    int title_x = (CHAR_COLS - strlen(title)) / 2;
    for (int i = 0; i < strlen(title); i++) {
        set_char(title_x + i, 2, title[i]);
        set_colour(title_x + i, 2, 0x3f, 0x00); // Branco
    }

    // 3. Exibe Valores Atuais (Texto Grande/Colorido)
    char buf[32];
    sprintf(buf, "TEMP: %.2f C", current_temp);
    for (int i = 0; i < strlen(buf); i++) {
        set_char(4 + i, 4, buf[i]);
        set_colour(4 + i, 4, 0x03, 0x00); // Vermelho (simulado)
    }

    sprintf(buf, "HUM: %.2f %%", current_hum);
    int hum_x = CHAR_COLS - 4 - strlen(buf);
    for (int i = 0; i < strlen(buf); i++) {
        set_char(hum_x + i, 4, buf[i]);
        set_colour(hum_x + i, 4, 0x30, 0x00); // Azul (simulado)
    }

    

    // Atualiza Mínimos e Máximos
    if(current_temp > max_temp) { max_temp = current_temp; }
    if(current_temp < min_temp) { min_temp = current_temp; }
    if(current_hum > max_hum) { max_hum  = current_hum; }
    if(current_hum < min_hum) { min_hum  = current_hum; }


    // 4. Exibe Estatísticas (Max/Min)
    char buf_stats[40];
    
    // Stats Temp (Linha 6 e 7)
    sprintf(buf_stats, "MAX: %.1f C", max_temp);
    for (int i = 0; i < strlen(buf_stats); i++) {
        set_char(4 + i, 6, buf_stats[i]);
        set_colour(4 + i, 6, 0x15, 0x00); // Cinza Claro
    }
    sprintf(buf_stats, "MIN: %.1f C", min_temp);
    for (int i = 0; i < strlen(buf_stats); i++) {
        set_char(4 + i, 7, buf_stats[i]);
        set_colour(4 + i, 7, 0x15, 0x00); // Cinza Claro
    }

    // Stats Hum (Linha 6 e 7 - Direita)
    sprintf(buf_stats, "MAX: %.1f %%", max_hum);
    int stats_x = CHAR_COLS - 4 - 15; // Ajuste aproximado
    for (int i = 0; i < strlen(buf_stats); i++) {
        set_char(stats_x + i, 6, buf_stats[i]);
        set_colour(stats_x + i, 6, 0x15, 0x00); // Cinza Claro
    }
    sprintf(buf_stats, "MIN: %.1f %%", min_hum);
    for (int i = 0; i < strlen(buf_stats); i++) {
        set_char(stats_x + i, 7, buf_stats[i]);
        set_colour(stats_x + i, 7, 0x15, 0x00); // Cinza Claro
    }
}

static void mock_sensor_data(sensor_data_t *dados) {
    static float temp = 24.0f;
    static float hum  = 55.0f;
    static int dir_t  = 1;
    static int dir_h  = 1;

    // Varia temperatura entre 24 e 30
    temp += 0.05f * dir_t;
    if (temp > 30.0f || temp < 24.0f) {
        dir_t = -dir_t;
    }

    // Varia umidade entre 45 e 70
    hum += 0.1f * dir_h;
    if (hum > 70.0f || hum < 45.0f) {
        dir_h = -dir_h;
    }

    dados->temperature = temp;
    dados->humidity    = hum;
}

void init_dvi() {
    dvi0.timing  = &DVI_TIMING;
    dvi0.ser_cfg = picodvi_dvi_cfg;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    // Limpa tela inicial
    for (uint y = 0; y < CHAR_ROWS; y++) {
        for (uint x = 0; x < CHAR_COLS; x++) {
            set_char(x, y, ' ');
            set_colour(x, y, 0x00, 0x00);
        }
    }
    draw_border();
}

// Função principal do Core 0 (lógica principal)
int __not_in_flash("main") main() {
    vreg_set_voltage(VREG_VSEL);
    sleep_ms(10);
    set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

    stdio_init_all();
    
    // Checa motivo do reset
    if (watchdog_caused_reboot()) {
        printf("ALERTA: Reinicializacao causada pelo Watchdog!\n");
    } else {
        printf("Inicializacao limpa (Power On).\n");
    }

    // Configura Watchdog para 4000ms (4 segundos)
    // Tempo suficiente para latencias de vídeo e UART
    watchdog_enable(4000, 1);
    // --- CONFIGURAÇÃO DO BOTÃO BOOTSEL ---
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    init_dvi();

    // UART
    uart_init(UART_ID, UART_BAUDRATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    // uart_set_fifo_enabled(UART_ID, true);


    // Inicia Core 1
    hw_set_bits(&bus_ctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC1_BITS);
    multicore_launch_core1(core1_main);

    static sensor_data_t dados;
    static char rx_buffer[RX_BUFFER_SIZE];
    uint8_t index = 0;
    union {
        float f;
        uint32_t u;
    } conv;
    while (true) {
        #if USE_MOCK_DATA
            mock_sensor_data(&dados);

            // Atualiza display
            draw_dashboard(dados.temperature, dados.humidity);

            sleep_ms(500); 

        #else
        // Comando experimental para simular travamento
            while (uart_is_readable(UART_ID)) {
                gpio_put(LED_BLUE, 1);
                char c = uart_getc(UART_ID);

                if (c == '\n') {
                    rx_buffer[index] = '\0';
                    index = 0;
                    
                    // Digite "KILL" ou 'k' para travar o Core 0
                    if (strcmp(rx_buffer, "KILL") == 0 || rx_buffer[0] == 'k') {
                        printf("Simulando travamento (loop infinito)...\n");
                        while(true); // Loop infinito para forçar watchdog
                    }

                    float temp, hum;
                    if (sscanf(rx_buffer, "TEMP:%.2f;HUM:%.2f\n", &temp, &hum) == 2) {
                        // Atualiza display
                        draw_dashboard(temp, hum);
                    }
                }
                else if (index < RX_BUFFER_SIZE - 1) {
                    rx_buffer[index++] = c;
                }
                else {
                    // overflow defensivo
                    index = 0;
                }
            }
            gpio_put(LED_BLUE, 0);

            sleep_ms(1);
        #endif
        
        // --- GERENCIAMENTO DO WATCHDOG --- 
        // Apenas alimenta o watchdog se o Core 1 estiver ativo (heartbeat variando)
        static uint32_t last_heartbeat = 0;
        if (core1_heartbeat != last_heartbeat) {
            watchdog_update();
            last_heartbeat = core1_heartbeat;
        } else {
            // Se o Core 1 travou, o heartbeat não muda, watchdog não é alimentado -> Reset.
        }
    }
}


// ============ core 1 ====================
// Função para definir um caractere na tela
static inline void set_char(uint x, uint y, char c) {
    if (x >= CHAR_COLS || y >= CHAR_ROWS)
        return;
    charbuf[x + y * CHAR_COLS] = c;
}

// Função para definir a cor de um caractere (formato RGB222)
static inline void set_colour(uint x, uint y, uint8_t fg, uint8_t bg) {
    if (x >= CHAR_COLS || y >= CHAR_ROWS)
        return;
    uint char_index = x + y * CHAR_COLS;
    uint bit_index = char_index % 8 * 4;
    uint word_index = char_index / 8;
    for (int plane = 0; plane < 3; ++plane) {
        uint32_t fg_bg_combined = (fg & 0x3) | (bg << 2 & 0xc);
        colourbuf[word_index] = (colourbuf[word_index] & ~(0xfu << bit_index)) | (fg_bg_combined << bit_index);
        fg >>= 2;
        bg >>= 2;
        word_index += COLOUR_PLANE_SIZE_WORDS;
    }
}

// --- FUNÇÃO PARA DESENHAR A BORDA ---
void draw_border() {
    const uint8_t fg = 0x15; // Cor cinza (RGB222: 010101)
    const uint8_t bg = 0x0c; // Cor de fundo (Verde)

    // Cantos
    set_char(0, 0, '+');
    set_colour(0, 0, fg, bg);
    set_char(CHAR_COLS - 1, 0, '+');
    set_colour(CHAR_COLS - 1, 0, fg, bg);
    set_char(0, CHAR_ROWS - 1, '+');
    set_colour(0, CHAR_ROWS - 1, fg, bg);
    set_char(CHAR_COLS - 1, CHAR_ROWS - 1, '+');
    set_colour(CHAR_COLS - 1, CHAR_ROWS - 1, fg, bg);

    // Linhas horizontais
    for (uint x = 1; x < CHAR_COLS - 1; ++x) {
        set_char(x, 0, '-');
        set_colour(x, 0, fg, bg);
        set_char(x, CHAR_ROWS - 1, '-');
        set_colour(x, CHAR_ROWS - 1, fg, bg);
    }
    // Linhas verticais
    for (uint y = 1; y < CHAR_ROWS - 1; ++y) {
        set_char(0, y, '|');
        set_colour(0, y, fg, bg);
        set_char(CHAR_COLS - 1, y, '|');
        set_colour(CHAR_COLS - 1, y, fg, bg);
    }
}

// Função principal do Core 1 (renderização DVI)
void core1_main() {
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);

    while (true) {
        for (uint y = 0; y < FRAME_HEIGHT; ++y) {
            // Lógica de Duplicação Vertical (Fator 3)
            // Calcula a linha do pixel da fonte original (0 a 7), repetindo 3 vezes
            uint font_row = (y % FONT_CHAR_HEIGHT) / FONT_SCALE_FACTOR; 

            uint32_t *tmdsbuf;
            queue_remove_blocking(&dvi0.q_tmds_free, &tmdsbuf);
            for (int plane = 0; plane < 3; ++plane) {
                tmds_encode_font_2bpp(
                    (const uint8_t*)&charbuf[y / FONT_CHAR_HEIGHT * CHAR_COLS],
                    &colourbuf[y / FONT_CHAR_HEIGHT * (COLOUR_PLANE_SIZE_WORDS / CHAR_ROWS) + plane * COLOUR_PLANE_SIZE_WORDS],
                    tmdsbuf + plane * (FRAME_WIDTH / DVI_SYMBOLS_PER_WORD),
                    FRAME_WIDTH,
                    
                    (const uint8_t*)&font_8x8[font_row * FONT_N_CHARS] - FONT_FIRST_ASCII
                );
            }
            queue_add_blocking(&dvi0.q_tmds_valid, &tmdsbuf);
        }
        core1_heartbeat++;
    }
}

