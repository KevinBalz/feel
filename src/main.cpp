#include <iostream>
#include "Finger.hpp"
#include <thread>
#include <queue>
#define ASIO_STANDALONE
#include "asio.hpp"

#include <chrono>
#include <thread>

class SerialConnection;
void ReadingThread(SerialConnection& connection);
class SerialConnection
{
public:
	SerialConnection() :
		inputs(),
		worker(ReadingThread, std::ref(*this))
	{}

	~SerialConnection()
	{
		worker.join();
	}

	void PrintAllMessages()
	{
		std::lock_guard<std::mutex> lock(inputMutex);
		while (!inputs.empty())
		{
			std::cout << inputs.front() << std::endl;
			inputs.pop();
		}
	}
private:
	std::queue<std::string> inputs;
	std::thread worker;
	std::mutex inputMutex;

	friend void ReadingThread(SerialConnection& connection);
	friend void ReadSerial(SerialConnection& connection, asio::serial_port& serial, asio::streambuf& b);
};

void ReadSerial(SerialConnection& connection, asio::serial_port& serial, asio::streambuf& b)
{
	asio::async_read_until(serial, b, '#', [&](auto ec, auto s)
	{
		std::lock_guard<std::mutex> lock(connection.inputMutex);
		std::string command{
				asio::buffers_begin(b.data()), asio::buffers_begin(b.data()) + s - 1};
		b.consume(s);
		connection.inputs.push(command);
		ReadSerial(connection, serial, b);
	});
}

void ReadingThread(SerialConnection& connection)
{
	std::cout << "Hello Feel" << std::endl;
	asio::io_service io;
	asio::serial_port serial(io, "\\\\.\\COM3");
	serial.set_option(asio::serial_port::baud_rate(115200));
	serial.set_option(asio::serial_port::character_size(8));
	asio::streambuf b;
	ReadSerial(connection, serial, b);
	
	io.run();
}



int main()
{
	SerialConnection con;

	for (;;)
	{
		std::cout << "PP" << std::endl;
		con.PrintAllMessages();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

    return 0;
}