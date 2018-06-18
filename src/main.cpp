#include <iostream>
#include "Finger.hpp"
#define ASIO_STANDALONE
#include "asio.hpp"

int main()
{
    std::cout << "Hello Feel" << std::endl;
    asio::io_service io;
    asio::serial_port serial(io, "/dev/tty.usbmodem1D151");
    serial.set_option(asio::serial_port::baud_rate(9600));
    serial.set_option(asio::serial_port::character_size(8));
    char c = '\0';
    std::string f = "1337\n";
    std::cout << f  << std::endl;
    asio::write(serial, asio::buffer(f, f.length()));
    while (c != EOF)
    {
        asio::read(serial, asio::buffer(&c, 1));
        std::cout << c << std::endl;
    }

    return 0;
}