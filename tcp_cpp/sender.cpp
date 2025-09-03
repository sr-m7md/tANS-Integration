#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <map>
#include "tANS.h"

using boost::asio::ip::tcp;

std::string read_text_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + filename);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345));

        std::string original_message = read_text_file("example.txt");
        if (original_message.empty()) throw std::runtime_error("Input file is empty");

        std::cout << "Original message size: " << original_message.size() << " bytes\n";
        
        std::map<char, size_t> freq;
        for (char c : original_message) freq[c]++;

        std::string alphabet;
        std::vector seen(256, false);
        for (char c : original_message) {
            if (!seen[static_cast<unsigned char>(c)]) {
                alphabet += c;
                seen[static_cast<unsigned char>(c)] = true;
            }
        }

        t_ans tans(alphabet);  
        tans.print_stats();

        std::cout << "Encoding...\n";
        auto start = std::chrono::high_resolution_clock::now();
        auto encode_result = tans.encode(original_message, 0);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed_seconds = end - start;
        std::cout << "Encoding took: " << elapsed_seconds.count() << " seconds.\n";
        std::cout << "Bitstream size: " << encode_result.bitstream.size() << " bits\n";
        std::cout << "Compression ratio: " << (double)encode_result.bitstream.size() / (original_message.size() * 8) << "\n";

        uint64_t state64 = encode_result.state;
        boost::asio::write(socket, boost::asio::buffer(&state64, sizeof(state64)));

        uint32_t stream_size = static_cast<uint32_t>(encode_result.bitstream.size());
        boost::asio::write(socket, boost::asio::buffer(&stream_size, sizeof(stream_size)));
        boost::asio::write(socket, boost::asio::buffer(encode_result.bitstream));

        std::cout << "Successfully sent compressed data over TCP.\n";

    } catch (std::exception& e) {
        std::cerr << "Sender error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
