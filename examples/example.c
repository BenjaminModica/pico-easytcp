#include <stdlib.h>
#include <string.h>

#include "easytcp.h"

#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    TCP_SERVER_T *easytcp_state = easytcp_init();

    while(1) {
        sleep_ms(10000);
        uint8_t data = 0x24;
        bool sent = easytcp_send_data(easytcp_state, data);

        if (sent) {
            printf("Data sent: 0x%x\n", data);
        } else {
            printf("Data not sent, no client connected to server\n");
        }

        uint8_t recv_data[128];
        int nbr_of_data = easytcp_receive_data(easytcp_state, recv_data);

        for (int i = 0; i < nbr_of_data; i++) {
            printf("Data at %d: %c\n", i, recv_data[i]);
        }
    }
}
