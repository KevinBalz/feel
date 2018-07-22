#pragma once
#include "SerialDevice.hpp"
#include "Finger.hpp"
#include "IncomingMessage.hpp"
#include <iomanip>
#include <map>
#include <array>
#include <cassert>
#include <functional>
#include <string>

namespace feel
{
	class Feel
	{
	public:
		Feel()
		{
		}

		void Connect(const char* deviceName)
		{
			device.Connect(deviceName);
		}

        std::vector<std::string> GetAvailableDevices()
        {
            std::vector<std::string> devices;
            device.GetAvailableDevices(devices);
            return std::move(devices);
        }

        void GetAvailableDevices(std::vector<std::string>& devices)
        {
            device.GetAvailableDevices(devices);
        }

		void BeginSession()
        {
            device.TransmitMessage("BS");
        }

		void EndSession()
		{
			device.TransmitMessage("ES");
		}

		void SubscribeForFingerUpdates(bool active = true)
		{
			device.TransmitMessage("SS", active ? "1" : "0");
		}

		void SetFingerAngle(Finger finger, float angle)
		{
			int fingerNumber = static_cast<int>(finger);
			std::stringstream stream;
			stream
				<< std::setfill('0') << std::setw(2)
				<< std::hex << fingerNumber
				<< std::dec << angle;
			device.TransmitMessage("WF", stream.str());
		}

		float GetFingerAngle(Finger finger)
		{
			return fingerAngles[static_cast<int>(finger)];
		}

		void SetDebugLogCallback(std::function<void(std::string) > callback)
		{
			debugLogCallback = callback;
		}

		void ParseMessages()
		{
			device.IterateAllMessages([&](auto message)
			{
				static std::map<std::string, IncomingMessage> incomingMessageMap =
				{
					{ "UF", IncomingMessage::FingerUpdate },
					{ "DL", IncomingMessage::DebugLog }
				};
				if (message.length() < 2) return;
				switch (incomingMessageMap[message.substr(0, 2)])
				{
					case IncomingMessage::DebugLog:
						debugLogCallback(message.substr(2));
						break;
					case IncomingMessage::FingerUpdate:
						std::string fingerIdentifier = message.substr(2, 2);
						std::string fingerAngle = message.substr(4);
						std::cout << "Finger: " << fingerIdentifier << " Angle: " << fingerAngle << std::endl;
						int fingerIndex = std::stoul(fingerIdentifier, nullptr, 16);
						fingerAngles[fingerIndex] = std::stof(fingerAngle);
						break;
				}
			});
		}
	private:
		SerialDevice device;
		std::function<void(std::string)> debugLogCallback = [](auto s)
        {
			std::cout << s << std::endl;
		};
		std::array<float, FINGER_TYPE_COUNT> fingerAngles = {};
	};
}