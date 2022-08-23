#include <thread>
#include <iostream>
#include <asio.hpp>

#define IPADDRESS "127.0.0.1" // "192.168.1.64"
#define UDP_PORT 13251

using asio::ip::udp;
using asio::ip::address;

void Sender(std::string in) {
    asio::io_service io_service;
    udp::socket socket(io_service);
    udp::endpoint remote_endpoint = udp::endpoint(address::from_string(IPADDRESS), UDP_PORT);
    socket.open(udp::v4());

    std::error_code err;
    auto sent = socket.send_to(asio::buffer(in), remote_endpoint, 0, err);
    socket.close();
    std::cout << "Sent Payload --- " << sent << "\n";
}

struct Client {
    asio::io_service io_service;
    udp::socket socket{io_service};
    char recv_buffer[1024];
    udp::endpoint remote_endpoint;

    int count = 3;

    void handle_receive(const std::error_code& error, size_t bytes_transferred) {
        if (error) {
            std::cout << "Receive failed: " << error.message() << "\n";
            return;
        }
        std::cout << "Received: '";
        for(int i = 0; i < bytes_transferred; i++){
            std::cout << recv_buffer[i];
            }
        std::cout << "' (" << error.message() << ")\n";
        if (--count > 0) {
            std::cout << "Count: " << count << "\n";
            wait();
        }
    }

    void wait() {
        socket.async_receive_from(asio::buffer(recv_buffer),
            remote_endpoint,
        [this](std::error_code ec, std::size_t bytes_recvd)
        {
          handle_receive(ec, bytes_recvd);
        });
    }

    void Receiver()
    {
        socket.open(udp::v4());
        socket.bind(udp::endpoint(address::from_string(IPADDRESS), UDP_PORT));

        wait();

        std::cout << "Receiving\n";
        io_service.run();
        std::cout << "Receiver exit\n";
    }
};

int main(int argc, char *argv[])
{
    Client client;
    std::thread r([&] { client.Receiver(); });

    std::string input = argc>1? argv[1] : "hello world";
    std::cout << "Input is '" << input.c_str() << "'\nSending it to Sender Function...\n";

    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        Sender(input);
    }

    r.join();
}