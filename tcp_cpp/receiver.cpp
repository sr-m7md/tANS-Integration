#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include "tANS.h"

using boost::asio::ip::tcp;

void save_to_file(const std::string& filename, const std::string& content) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot create output file: " + filename);
    }
    file.write(content.data(), content.size());
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));

        std::cout << "Waiting for connection on port 12345..." << std::endl;
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Connection accepted!" << std::endl;

        std::string full_byte_alphabet(256, '\0');
        for (int i = 0; i < 256; ++i) {
            full_byte_alphabet[i] = static_cast<char>(i);
        }

        uint64_t state_64;
        boost::asio::read(socket, boost::asio::buffer(&state_64, sizeof(state_64)));
        size_t state = static_cast<size_t>(state_64);
        std::cout << "Received state: " << state << std::endl;

        uint32_t stream_size;
        boost::asio::read(socket, boost::asio::buffer(&stream_size, sizeof(stream_size)));
        std::cout << "Expected bitstream size: " << stream_size << " bits" << std::endl;

        std::string bitstream;
        if (stream_size > 0) {
            bitstream.resize(stream_size);
            boost::asio::read(socket, boost::asio::buffer(bitstream));
        }

        std::cout << "Received bitstream of size: " << bitstream.size() << std::endl;

        const t_ans tans(full_byte_alphabet);
        tans.print_stats();

        std::cout << "Decoding..." << std::endl;
        const auto start = std::chrono::high_resolution_clock::now();
        const auto decode_result = tans.decode(state, bitstream);
        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed_seconds = end - start;

        std::cout << "Decoding took: " << elapsed_seconds.count() << " seconds." << std::endl;
        std::cout << "Decoded message size: " << decode_result.message.size() << " bytes" << std::endl;
        std::cout << "Final state: " << decode_result.final_state << std::endl;

        save_to_file("decoded_output.txt", decode_result.message);
        std::cout << "Decoded message saved to 'decoded_output.txt'" << std::endl;

        if (!decode_result.message.empty()) {
            std::cout << "\nFirst 200 characters of decoded message:" << std::endl;
            std::string preview = decode_result.message.substr(0, std::min(200UL, decode_result.message.size()));

            std::string printable_preview;
            for (char c : preview) {
                if (std::isprint(c)) {
                    printable_preview += c;
                } else if (c == '\n') {
                    printable_preview += "\\n";
                } else if (c == '\t') {
                    printable_preview += "\\t";
                } else if (c == '\r') {
                    printable_preview += "\\r";
                } else {
                    printable_preview += "\\x" +
                        std::string(1, "0123456789ABCDEF"[(c >> 4) & 0xF]) +
                        std::string(1, "0123456789ABCDEF"[c & 0xF]);
                }
            }
            std::cout << printable_preview << std::endl;

            if (decode_result.message.size() > 200) {
                std::cout << "... (truncated)" << std::endl;
            }
        }

        std::cout << "\nDecoding completed successfully!" << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Receiver error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
