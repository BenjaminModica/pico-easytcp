/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * An "interface" to make project code cleaner by abstraction
 * when using WIFI on pico and TCP communication through the LWIP library.
 * 
 * Based on pico-examples from Raspberry Pi
 * 
 * Written by: Benjamin Modica 2023
 */

#include "easytcp.h"
#include "secrets.h"

/**
 * Init for easytcp library and settings
*/
TCP_SERVER_T* easytcp_init() {

    if (cyw43_arch_init()) {
        printf("failed to initialise CYW43 architecture\n");
        return NULL;
    }

    cyw43_arch_enable_sta_mode(); //Enable wifi station mode

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return NULL;
    } else {
        printf("Connected.\n");
    }

    TCP_SERVER_T *state= run_tcp_server();
     if (!state) {
        printf("Failed to run TCP server");
        return NULL;
    }

    state->ringbuf_read = 0;
    state->ringbuf_write = 0;
    
    return state;
}

/**
 * De-initialize easytcp, what more to do? 
*/
int easytcp_deinit(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }

    cyw43_arch_deinit();
    free(state);
}

/**
 * For putting received data in ringbuffer.
 * If buffer overflows it should start placing data in 
 * the beginning of the array again. 
 * 
 * Increments ringbuf write with one for each data.
 * 
 * Data will be overwritten if it comes full circuit without being emptied. 
 * So this will only save the latest RINGBUF_SIZE entries which is fine for my use cases. 
 * 
 * @param arg Pointer to tcp state structure
 * @param data Data to store in buffer
*/
void put_ringbuffer(void *arg, uint8_t data) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    state->ringbuffer[state->ringbuf_write] = data;
    state->ringbuf_write++;
    if (state->ringbuf_write >= RINGBUF_SIZE) {
        state->ringbuf_write = 0;
    } 
}

/**
 * For reading the data in the ringbuffer, emptying it. Should reset ringbuffer.
 * 
 * Will read out the data until read pointer has reached write pointer 
 * 
 * @param arg Pointer to tcp state structure
 * @param data Pointer to array in which data gets returned
 * 
 * @returns How many bytes of data has been written
*/
int read_ringbuffer(void *arg, uint8_t *data) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    //Empty ringbuffer into data
    int i = 0;
    while(state->ringbuf_read != state->ringbuf_write) {

        if (state->ringbuf_read >= RINGBUF_SIZE) {
            state->ringbuf_read = 0;
        }

        data[i] = state->ringbuffer[state->ringbuf_read];
        state->ringbuf_read++;
        i++;
    }

    return i;
}

/**
 * Function for sending data to client
 * 
 * Returns true if data was sent
 * Returns false if no client is connected to receive data
*/
bool easytcp_send_data(void *arg, uint8_t data) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (is_client_connected(state)){
        tcp_server_send_data(state, state->client_pcb, data);
        return true;
    } else {
        printf("No client connected, cannot send data\n");
        return false;
    }
}

bool is_client_connected(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if  (state->client_pcb != NULL) {
        printf("client connected\n");
        return true;
    } else {
        printf("client not connected\n");
        return false;
    }
}

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
            printf("close failed %d, calling abort\n", err);
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
        printf("Something failed or client disconnected %d\n", status);
    }
    return tcp_client_close(arg);
}

/**
 * Callback function when data has been sent
 * 
 * This is just unneccesary now, might be useful later.
*/
err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    printf("tcp_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE_SENT) {
        state->recv_len = 0;
    }

    return ERR_OK;
}

/**
 * Callback function when data has been received
 * 
 * Currently can only manage to receive one byte at a time. Could be extended in future.
*/
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        printf("No main packet buffer struct\n\r");
        return tcp_server_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called then
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        //printf("tcp_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE_RECV - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    //Check if the whole buffer is received
    if (state->recv_len == BUF_SIZE_RECV) {
        printf("Received Buffer: %c\n", state->buffer_recv[0]);
    }

    put_ringbuffer(state, state->buffer_recv[0]);

    state->recv_len = 0;

    return ERR_OK;

}

/**
 * Function for sending data to client
*/
err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb, uint8_t data) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    //Prepare data to be sent
    memcpy(state->buffer_sent, &data, BUF_SIZE_SENT);

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
    return tcp_server_result(arg, 0); // no response is no error?
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

err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
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
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S); //Uncomment for timeout on response from client.
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;

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