#include <stdlib.h>
#include <string.h>

#include "easytcp.h"

#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    TCP_SERVER_T *easytcp_state = easytcp_init();

    while(1) {
        sleep_ms(500);
        uint8_t data = 0x25;
        easytcp_send_data(easytcp_state, data);
    }
}