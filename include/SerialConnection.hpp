#include "asio.hpp"
#include <thread>
#include <queue>
#include <iostream>

class SerialConnection;
void ReadingThread(SerialConnection& connection);
class SerialConnection
{
public:
	SerialConnection() :
		inputs(),
		io(),
		serial(io),
		worker(ReadingThread, std::ref(*this))
	{}

	~SerialConnection()
	{
		io.stop();
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

	void SubscribeForFingerUpdates(bool active = true)
	{
		SendSerialMessage("SS", active ? "1" : "0");
	}

	void SendSerialMessage(std::string identifier, std::string payload)
	{
		auto str = std::make_shared<std::string>(identifier + payload + "#");
		asio::async_write(serial, asio::buffer(*str), [str](auto ec, auto s)
		{
		});
	}

private:
	asio::io_service io;
	asio::serial_port serial;
	std::queue<std::string> inputs;
	std::thread worker;
	std::mutex inputMutex;

	friend void ReadingThread(SerialConnection& connection);
	friend void ReadSerial(SerialConnection& connection, asio::streambuf& b);
};

void ReadSerial(SerialConnection& connection, asio::streambuf& b)
{
	asio::async_read_until(connection.serial, b, '#', [&](auto ec, auto s)
	{
		std::string message
		{
			asio::buffers_begin(b.data()), asio::buffers_begin(b.data()) + s - 1
		};
		b.consume(s);
		std::lock_guard<std::mutex> lock(connection.inputMutex);
		connection.inputs.push(message);
		ReadSerial(connection, b);
	});
}

void ReadingThread(SerialConnection& connection)
{
	connection.serial.open("\\\\.\\COM3");
	connection.serial.set_option(asio::serial_port::baud_rate(115200));
	connection.serial.set_option(asio::serial_port::character_size(8));
	asio::streambuf b;
	ReadSerial(connection, b);

	connection.io.run();
}