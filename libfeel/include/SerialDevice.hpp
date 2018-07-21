#pragma once
#include "Device.hpp"
#define ASIO_STANDALONE
#include "asio.hpp"
#include <thread>
#include <queue>
#include <iostream>

namespace feel
{
	class SerialDevice;
	void ReadingThread(SerialDevice& connection, const char* port);
	class SerialDevice : public Device
	{
	public:
		SerialDevice() :
			inputs(),
			io(),
			serial(io)
		{}

		~SerialDevice()
		{
			if (connecting || connected)
			{
				io.stop();
				worker.join();
			}
		}

		void Connect(const char* deviceName) override
		{
			std::lock_guard<std::mutex> lock(connectionStatusMutex);
		    if (!(connected || connecting))
			{
				worker = std::thread(ReadingThread, std::ref(*this), deviceName);
				connecting = true;
			}
		}

		void IterateAllMessages(std::function<void(std::string)> callback) override
		{
			std::lock_guard<std::mutex> lock(inputMutex);
			while (!inputs.empty())
			{
				callback(inputs.front());
				inputs.pop();
			}
		}

		void TransmitMessage(std::string identifier, std::string payload = "") override
		{
			auto str = std::make_shared<std::string>(identifier + payload + "#");
			asio::async_write(serial, asio::buffer(*str), [str](auto ec, auto s)
			{
			});
		}

		void IterateAllLogs(std::function<void(std::string)> callback) override
		{}

	private:
		bool connected = false;
		bool connecting = false;
		asio::io_service io;
		asio::serial_port serial;
		std::queue<std::string> inputs;
		std::thread worker;
		std::mutex connectionStatusMutex;
		std::mutex inputMutex;

		friend void ReadingThread(SerialDevice& connection, const char* port);
		friend void ReadSerial(SerialDevice& connection, asio::streambuf& b);
	};

	void ReadSerial(SerialDevice& connection, asio::streambuf& b)
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

	void ReadingThread(SerialDevice& connection, const char* port)
	{
		//"/dev/cu.usbmodem1421"
		{
			std::lock_guard<std::mutex> lock(connection.connectionStatusMutex);
			try
			{
				connection.serial.open(port);
				connection.connected = true;
				connection.connecting = false;
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
				connection.connected = false;
				connection.connecting = false;
				return;
			}
		}
		connection.serial.set_option(asio::serial_port::baud_rate(115200));
		connection.serial.set_option(asio::serial_port::character_size(8));
		asio::streambuf b;
		ReadSerial(connection, b);

		connection.io.run();
		std::lock_guard<std::mutex> lock(connection.connectionStatusMutex);
		connection.connected = false;
		connection.connecting = false;
	}
}