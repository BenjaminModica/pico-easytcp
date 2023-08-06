/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * An "interface" to make project code cleaner by abstraction
 * when using TCP communication through the LWIP library.
 * 
 * Based on pico-examples from Raspberry Pi
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
#define BUF_SIZE_SENT 1
#define BUF_SIZE_RECV 1
#define POLL_TIME_S 60

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb; //Server protocol control block, containing information about the connection
    struct tcp_pcb *client_pcb; //Client protocol control block
    uint8_t buffer_sent[BUF_SIZE_SENT];
    uint8_t buffer_recv[BUF_SIZE_RECV]; 
    int sent_len;
    int recv_len;
    int run_count;
} TCP_SERVER_T;

/**
 * Allocate space for tcp struct and return pointer to struct
*/
TCP_SERVER_T* tcp_server_init() {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

/**
 * Function for closing connection to client
 * Keeps server alive
*/
err_t tcp_client_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    /* This is for also closing the server
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    */
    return err;
}

/**
 * Might not need this function? Skip directly to close function? 
*/
err_t tcp_server_result(void *arg, int status) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (status == 0) {
        printf("Closed tcp server succesfully\n");
    } else {
        printf("Something failed %d\n", status);
    }
    return tcp_client_close(arg);
}

/**
 * Callback function when data has been sent
*/
err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    printf("tcp_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE_SENT) {

        // We should get the data back from the client
        state->recv_len = 0;
        printf("Waiting for buffer from client\n");
    }

    return ERR_OK;
}

/**
 * Callback function when data has been received
*/
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {}

/**
 * Function for sending data to client
*/
err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    //Prepare data to be sent
    memcpy(state->buffer_sent, "b", BUF_SIZE_SENT);

    state->sent_len = 0;
    printf("Writing %ld bytes to client\n", BUF_SIZE_SENT);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_sent, BUF_SIZE_SENT, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Failed to write data %d\n", err);
        return tcp_server_result(arg, -1);
    }
    return ERR_OK;
}

/**
 * Callback function for timeout when polling client
*/
err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    printf("tcp_server_poll_fn\n");
    return tcp_server_result(arg, -1); // no response is an error?
}

/**
 * Callback function for when fatal error has occured
*/
void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        printf("tcp_client_err_fn %d\n", err);
        tcp_server_result(arg, err);
    }
}

void tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        printf("Failure in accept\n");
        tcp_server_result(arg, err);
        return ERR_VAL;
    }
    printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    //tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2); //Uncomment for timeout on response from client.
    tcp_err(client_pcb, tcp_server_err);

    //Attempt to write this function without returning err_t
    //return tcp_server_send_data(arg, state->client_pcb);
}

/**
 * initializes and configures the TCP server by creating a PCB, 
 * binding it to a port, setting up the server PCB, 
 * and configuring callback functions to accept connections 
*/
bool tcp_server_open(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        printf("failed to bind to port %u\n", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
        if (!state->server_pcb) {
        printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

/**
 * Initialize TCP server
 * Return if something goes wrong
*/
TCP_SERVER_T* run_tcp_server() {
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) {
        return NULL;
    }
    if (!tcp_server_open(state)) {
        return NULL;
    }
    return state; 
}

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

    TCP_SERVER_T *state = run_tcp_server();
     if (!state) {
        printf("Failed to run TCP server");
        return 1;
    }

    while(1) {
        printf("Hello\n");
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        if (state->client_pcb != NULL) {
            tcp_server_send_data(state, state->client_pcb);
        }
    }

    cyw43_arch_deinit();

    return 0;
}
