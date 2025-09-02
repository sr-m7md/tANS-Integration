#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "tans_embedded.h"
#include "net/linkaddr.h"
#include "sys/node-id.h"
#include <string.h>

#define LOG_MODULE "tANS Sender"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678
#define SEND_INTERVAL (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static tans_t tans_encoder;

typedef struct {
    uint16_t state;
    uint16_t bitstream_size;
    uint8_t bitstream[MAX_BITSTREAM_SIZE / 8];
} tans_packet_t;

PROCESS(sender_process, "tANS Sender Process");
AUTOSTART_PROCESSES(&sender_process);

static void init_tans_encoder(void) {
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

    int result = tans_init(&tans_encoder, text_alphabet, 64);
    if (result != TANS_OK) {
        LOG_ERR("Failed to initialize tANS encoder: %d\n", result);
    } else {
        LOG_INFO("tANS encoder initialized successfully\n");
        tans_print_stats(&tans_encoder);
    }
}

PROCESS_THREAD(sender_process, ev, data) {
    static struct etimer periodic_timer;
    uip_ipaddr_t dest_ipaddr;
    static const char* test_messages[] = {
        "hello world",
        "this is a test message",
        "contiki os with tans compression",
        "iot sensor data transmission",
        "embedded systems are cool"
    };
    static int message_index = 0;

    PROCESS_BEGIN();

    node_id = 1801;
    LOG_INFO("tANS Sender starting\n");

    init_tans_encoder();

    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, NULL);

    uip_ip6addr(&dest_ipaddr, 0xfd00, 0, 0, 0, 0x302, 0x304, 0x506, 0x709);

    etimer_set(&periodic_timer, SEND_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));


        const char* message = test_messages[message_index];
        message_index = (message_index + 1) % 5;

        LOG_INFO("Encoding message: '%s' (%u bytes)\n",
                 message, (unsigned)strlen(message));

        tans_encode_result_t encoded_result;
        int result = tans_encode(&tans_encoder,
                               (const uint8_t*)message,
                               strlen(message),
                               0,
                               &encoded_result);

        if (result == TANS_OK) {

            tans_packet_t packet;
            packet.state = encoded_result.state;
            packet.bitstream_size = encoded_result.bitstream_size;


            memset(packet.bitstream, 0, sizeof(packet.bitstream));

            for (int i = 0; i < encoded_result.bitstream_size; i++) {
                int byte_idx = i / 8;
                int bit_idx = i % 8;

                if (encoded_result.bitstream[byte_idx] & (1 << (7 - bit_idx))) {
                    packet.bitstream[byte_idx] |= (1 << (7 - bit_idx));
                }
            }

            LOG_INFO("Sending to receiver...\n");
            int bytes_sent = simple_udp_sendto(&udp_conn, &packet,
                                              sizeof(packet), &dest_ipaddr);

            if (bytes_sent > 0) {
                LOG_INFO("Sent tANS packet: %u bytes original -> %u bits compressed (state=%u)\n",
                         (unsigned)strlen(message),
                         encoded_result.bitstream_size,
                         encoded_result.state);

                float compression_ratio = (float)encoded_result.bitstream_size / (strlen(message) * 8);
                LOG_INFO("Compression ratio: %.2f\n", compression_ratio);
            } else {
                LOG_ERR("Failed to send UDP packet\n");
            }

        } else {
            LOG_ERR("tANS encoding failed: %d\n", result);
        }

        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}