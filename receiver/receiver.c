#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "tans_embedded.h"
#include <string.h>

#define LOG_MODULE "tANS Receiver"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_SERVER_PORT 5678

static struct simple_udp_connection udp_conn;
static tans_t tans_decoder;

typedef struct {
    uint16_t state;
    uint16_t bitstream_size;
    uint8_t bitstream[MAX_BITSTREAM_SIZE / 8];
} tans_packet_t;

PROCESS(receiver_process, "tANS Receiver Process");
AUTOSTART_PROCESSES(&receiver_process);

static void init_tans_decoder(void) {
    uint8_t text_alphabet[64];
    int idx = 0;

    for (int i = 0; i < 20; i++) text_alphabet[idx++] = ' ';

    for (int i = 0; i < 3; i++) {
        text_alphabet[idx++] = 'a';
        text_alphabet[idx++] = 'e';
        text_alphabet[idx++] = 'i';
        text_alphabet[idx++] = 'o';
        text_alphabet[idx++] = 'u';
    }

    const char consonants[] = "nrtlsdhcfmpgwybvkjxqz";
    for (int i = 0; i < strlen(consonants) && idx < 64; i++) {
        text_alphabet[idx++] = consonants[i];
    }

    while (idx < 64) {
        text_alphabet[idx++] = '.';
    }

    int result = tans_init(&tans_decoder, text_alphabet, 64);
    if (result != TANS_OK) {
        LOG_ERR("Failed to initialize tANS decoder: %d\n", result);
    } else {
        LOG_INFO("tANS decoder initialized successfully\n");
        tans_print_stats(&tans_decoder);
    }
}

static void udp_rx_callback(struct simple_udp_connection *c,
                           const uip_ipaddr_t *sender_addr,
                           uint16_t sender_port,
                           const uip_ipaddr_t *receiver_addr,
                           uint16_t receiver_port,
                           const uint8_t *data,
                           uint16_t datalen) {

    LOG_INFO("Received UDP packet from ");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_(" port %u, length %u\n", sender_port, datalen);

    if (datalen == sizeof(tans_packet_t)) {
        tans_packet_t *packet = (tans_packet_t*)data;

        LOG_INFO("Received tANS packet: state=%u, bitstream_size=%u bits\n",
                 packet->state, packet->bitstream_size);

        tans_decode_result_t decode_result;
        int result = tans_decode(&tans_decoder,
                                packet->state,
                                packet->bitstream,
                                packet->bitstream_size,
                                &decode_result);

        if (result == TANS_OK) {

            char decoded_message[MAX_MESSAGE_SIZE + 1];
            memcpy(decoded_message, decode_result.message, decode_result.message_size);
            decoded_message[decode_result.message_size] = '\0';

            LOG_INFO("Successfully decoded message: '%s' (%u bytes)\n",
                     decoded_message, decode_result.message_size);

            float compression_ratio = (float)packet->bitstream_size / (decode_result.message_size * 8);
            LOG_INFO("Compression ratio: %.2f\n", compression_ratio);
            LOG_INFO("Final state: %u\n", decode_result.final_state);

        } else {
            LOG_ERR("tANS decoding failed: %d\n", result);
        }

    } else {
        LOG_WARN("Received packet with unexpected size: %u (expected %u)\n",
                 datalen, (unsigned)sizeof(tans_packet_t));
    }
}

PROCESS_THREAD(receiver_process, ev, data) {
    PROCESS_BEGIN();

    LOG_INFO("tANS Receiver starting\n");

    init_tans_decoder();

    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, 0, udp_rx_callback);

    LOG_INFO("Waiting for tANS packets on UDP port %u\n", UDP_SERVER_PORT);

    while(1) {
        PROCESS_YIELD();
    }

    PROCESS_END();
}