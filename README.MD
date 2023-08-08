# Easy TCP - For Raspberry Pi Pico

Library for TCP connections when using the pico microcontroller.

I got tired of working with the TCP examples in [pico-examples](https://github.com/raspberrypi/pico-examples) for projects. So I made this library to make TCP projects easier by abstraction. 

Sets up the pico as a TCP server in the same way as tcp_server in [pico-examples](https://github.com/raspberrypi/pico-examples). Only allows for sending and receiving 1 byte at a time, but saves received bytes in a buffer.

**Functions**
 - easytcp_init() - Initiates WiFi connection and sets up TCP server.
 - easytcp_send_data() - Sends 1 byte of data at a time.
 - easytcp_receive_data() - Receives a buffer of RINGBUF_SIZE length with data that the server has received. 

I will gladly accept contributions or suggestions! Because I really don't know what I'm doing half the time...