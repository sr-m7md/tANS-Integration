#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "tans_embedded.h"
#include <string.h>

#define LOG_MODULE "tANS Combined"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678
#define SEND_INTERVAL (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static tans_t tans_encoder;
static tans_t tans_decoder;

typedef struct {
    uint16_t state;
    uint16_t bitstream_size;
    uint8_t bitstream[MAX_BITSTREAM_SIZE / 8];
} tans_packet_t;

PROCESS(tans_combined_process, "tANS Combined Process");
AUTOSTART_PROCESSES(&tans_combined_process);

static void init_tans_alphabet(uint8_t *alphabet) {
    int idx = 0;

    // 20 пробелов для высокой частоты
    for (int i = 0; i < 20; i++) alphabet[idx++] = ' ';

    // 3 повтора гласных для средней частоты
    for (int i = 0; i < 3; i++) {
        alphabet[idx++] = 'a';
        alphabet[idx++] = 'e';
        alphabet[idx++] = 'i';
        alphabet[idx++] = 'o';
        alphabet[idx++] = 'u';
    }

    // Согласные по частоте использования
    const char consonants[] = "nrtlsdhcfmpgwybvkjxqz";
    for (int i = 0; i < strlen(consonants) && idx < 64; i++) {
        alphabet[idx++] = consonants[i];
    }

    // Заполняем оставшиеся места
    while (idx < 64) {
        alphabet[idx++] = '.';
    }
}

static void init_tans_encoder(void) {
    uint8_t text_alphabet[64];
    init_tans_alphabet(text_alphabet);

    int result = tans_init(&tans_encoder, text_alphabet, 64);
    if (result != TANS_OK) {
        LOG_ERR("Failed to initialize tANS encoder: %d\n", result);
    } else {
        LOG_INFO("tANS encoder initialized successfully\n");
        tans_print_stats(&tans_encoder);
    }
}

static void init_tans_decoder(void) {
    uint8_t text_alphabet[64];
    init_tans_alphabet(text_alphabet);

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
            LOG_INFO("Decompression ratio: %.2f\n", compression_ratio);
            LOG_INFO("Final state: %u\n", decode_result.final_state);

        } else {
            LOG_ERR("tANS decoding failed: %d\n", result);
        }

    } else {
        LOG_WARN("Received packet with unexpected size: %u (expected %u)\n",
                 datalen, (unsigned)sizeof(tans_packet_t));
    }
}

PROCESS_THREAD(tans_combined_process, ev, data) {
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

    LOG_INFO("tANS Combined Sender/Receiver starting\n");

    // Инициализация encoder и decoder
    init_tans_encoder();
    init_tans_decoder();

    // Регистрация UDP соединения (слушаем на SERVER_PORT, отправляем с CLIENT_PORT)
    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);

    // Устанавливаем адрес назначения (отправляем себе для тестирования)
    uip_ip6addr(&dest_ipaddr, 0xfd00, 0, 0, 0, 0x302, 0x304, 0x506, 0x708);

    LOG_INFO("Waiting for tANS packets on UDP port %u\n", UDP_SERVER_PORT);
    LOG_INFO("Will send tANS packets from port %u\n", UDP_CLIENT_PORT);

    etimer_set(&periodic_timer, SEND_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        // Отправка сообщения
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

            // Копирование битстрима
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

            if (bytes_sent >= 0) {
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