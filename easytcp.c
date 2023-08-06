/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * An "interface" to make project code cleaner by abstraction
 * when using TCP communication through the LWIP library.
 * 
 * Written by: Benjamin Modica 2023
 */

#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "secrets.h"

#define TCP_PORT 4242
#define BUF_SIZE 2048

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb; //Server protocol control block, containing information about the connection
    struct tcp_pcb *client_pcb; //Client protocol control block
    uint8_t buffer_sent[BUF_SIZE];
    uint8_t buffer_recv[BUF_SIZE]; 
    int sent_len;
    int recv_len;
    int run_count;
} TCP_SERVER_T;

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise CYW43 architecture\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode(); //Enable wifi station mode

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
    }

    while(1) {
        printf("Hello\n");
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    }

    cyw43_arch_deinit();

    return 0;
}
