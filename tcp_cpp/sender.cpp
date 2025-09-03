#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include "tANS.h"

using boost::asio::ip::tcp;

std::string read_text_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);

        std::cout << "Connecting to receiver..." << std::endl;
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345));
        std::cout << "Connected successfully!" << std::endl;

        std::string full_byte_alphabet(256, '\0');
        for (int i = 0; i < 256; ++i) {
            full_byte_alphabet[i] = static_cast<char>(i);
        }

        std::string original_message = read_text_file("example.txt");
        if (original_message.empty()) {
            throw std::runtime_error("Input file is empty");
        }

        std::cout << "Original message size: " << original_message.size() << " bytes" << std::endl;

        const t_ans tans(full_byte_alphabet);
        tans.print_stats();

        std::cout << "Encoding..." << std::endl;
        const auto start = std::chrono::high_resolution_clock::now();
        const auto encode_result = tans.encode(original_message, 0);
        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed_seconds = end - start;

        std::cout << "Encoding took: " << elapsed_seconds.count() << " seconds." << std::endl;
        std::cout << "Encoded state: " << encode_result.state << std::endl;
        std::cout << "Bitstream size: " << encode_result.bitstream.size() << " bits" << std::endl;
        std::cout << "Compression ratio: " << (double)encode_result.bitstream.size() / (original_message.size() * 8) << std::endl;

        uint64_t state = static_cast<uint64_t>(encode_result.state);
        boost::asio::write(socket, boost::asio::buffer(&state, sizeof(state)));

        uint32_t stream_size = static_cast<uint32_t>(encode_result.bitstream.size());
        boost::asio::write(socket, boost::asio::buffer(&stream_size, sizeof(stream_size)));

        if (stream_size > 0) {
            boost::asio::write(socket, boost::asio::buffer(encode_result.bitstream));
        }

        std::cout << "Successfully sent compressed data over TCP." << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Sender error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
