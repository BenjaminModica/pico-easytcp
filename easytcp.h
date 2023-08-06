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

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define TCP_PORT 4242
#define BUF_SIZE_SENT 1
#define BUF_SIZE_RECV 1
#define POLL_TIME_S 255

extern char WIFI_PASSWORD[];
extern char WIFI_SSID[];

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb; //Server protocol control block, containing information about the connection
    struct tcp_pcb *client_pcb; //Client protocol control block
    uint8_t buffer_sent[BUF_SIZE_SENT];
    uint8_t buffer_recv[BUF_SIZE_RECV];
    int sent_len;
    int recv_len;
    int run_count;
} TCP_SERVER_T;

//TCP_SERVER_T *tcp_server_ptr;

bool easytcp_send_data(void *arg, uint8_t data);
bool is_client_connected(void *arg);
TCP_SERVER_T* tcp_server_init();
int easytcp_deinit(void *arg);
TCP_SERVER_T* easytcp_init();
err_t tcp_client_close(void *arg);
err_t tcp_server_result(void *arg, int status);
err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb, uint8_t data);
err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb);
void tcp_server_err(void *arg, err_t err);
err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
bool tcp_server_open(void *arg);
TCP_SERVER_T* run_tcp_server();

#endif // TCP_SERVER_H
