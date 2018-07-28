#pragma once
#include "Device.hpp"
#define ASIO_STANDALONE
#include "asio.hpp"
#include <thread>
#include <queue>
#include <iostream>
#include <atomic>
#include <Windows.h>
#include <winreg.h>

namespace feel
{
	class SerialDevice;
	void ReadingThread(SerialDevice& connection);
    void WritingThread(SerialDevice& connection);
	class SerialDevice : public Device
	{
	public:
		SerialDevice() :
			inputs(),
			io(),
			serial(io),
            status(DeviceStatus::Disconnected)
		{}

		~SerialDevice()
		{
            if (writeWorker.joinable())
            {
                writeWorker.join();
            }
            if (readWorker.joinable())
            {
                readWorker.join();
            }
		}

        DeviceStatus GetStatus() override
        {
            return status;
        }

		void Connect(const char* deviceName) override
		{
		    if (status == DeviceStatus::Disconnected)
			{
                status = DeviceStatus::Connecting;
                try
                {
                    serial.open(deviceName);
                    serial.set_option(asio::serial_port::baud_rate(115200));
                    serial.set_option(asio::serial_port::character_size(8));
                    status = DeviceStatus::Connected;
                }
                catch (const std::exception& e)
                {
                    std::cout << e.what() << std::endl;
                    status = DeviceStatus::Disconnected;
                    outputCondition.notify_one();
                    return;
                }
                readWorker = std::thread(ReadingThread, std::ref(*this));
                writeWorker = std::thread(WritingThread, std::ref(*this));
			}
		}

        void Disconnect() override
        {
            if (status != DeviceStatus::Disconnected)
            {
                status = DeviceStatus::Disconnected;
                outputCondition.notify_one();
                writeWorker.join();
                serial.cancel();
            }
        }

        void GetAvailableDevices(std::vector<std::string>& devices) override
        {
            LSTATUS lstatus;
            HKEY key;
            lstatus = RegOpenKey(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", &key);
            if (lstatus == ERROR_FILE_NOT_FOUND) return;
            char valueNameBuffer[1024];
            char valueBuffer[1024];
            DWORD index = 0;
            while (true)
            {
                DWORD bufferSize = 1024;
                DWORD valueSize = 1024;
                lstatus = RegEnumValue(key, index, valueNameBuffer, &bufferSize, NULL, NULL, NULL, NULL);
                if (lstatus == ERROR_NO_MORE_ITEMS) break;
                lstatus = RegGetValue(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", valueNameBuffer, RRF_RT_REG_SZ, NULL, valueBuffer, &valueSize);
                devices.emplace_back(valueBuffer);
                index++;
            }
        }

		void IterateAllMessages(std::function<void(const std::string&)> callback) override
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
			auto str = identifier + payload + "#";
            std::lock_guard<std::mutex> lock(outputMutex);
            outputs.push(str);
            outputCondition.notify_one();
		}

		void IterateAllLogs(std::function<void(std::string)> callback) override
		{}

	private:
        DeviceStatus status;
		asio::io_service io;
		asio::serial_port serial;
		std::queue<std::string> inputs;
        std::queue<std::string> outputs;
		std::thread readWorker;
        std::thread writeWorker;
		std::mutex inputMutex;
        std::mutex outputMutex;
        std::condition_variable outputCondition;

		friend void ReadingThread(SerialDevice& connection);
		friend void ReadSerial(SerialDevice& connection, asio::streambuf& b);
        friend void WritingThread(SerialDevice& connection);
	};

	void ReadSerial(SerialDevice& connection, asio::streambuf& b)
	{
		asio::async_read_until(connection.serial, b, '#', [&](auto ec, auto s)
		{
            if (!!ec) return;
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

	void ReadingThread(SerialDevice& connection)
	{
		asio::streambuf b;
		ReadSerial(connection, b);

		connection.io.run();
	}

    void WritingThread(SerialDevice& connection)
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(connection.outputMutex);
            connection.outputCondition.wait(lock, [&]
            {
                return !connection.outputs.empty() || connection.status == DeviceStatus::Disconnected;
            });
            if (connection.outputs.empty() && connection.status == DeviceStatus::Disconnected)
            {
                break;
            }

            std::string message = connection.outputs.front();
            connection.outputs.pop();
            lock.unlock();
            asio::write(connection.serial, asio::buffer(message));
        }
    }
}