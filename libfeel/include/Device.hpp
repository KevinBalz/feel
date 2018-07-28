#pragma once
#include <string>
#include <functional>
#include <vector>
#include "DeviceStatus.hpp"

namespace feel
{
	class Device
	{
	public:
        virtual DeviceStatus GetStatus() = 0;
		virtual void Connect(const char* deviceName) = 0;
        virtual void Disconnect() = 0;
        virtual void GetAvailableDevices(std::vector<std::string>& devices) = 0;
		virtual void TransmitMessage(std::string identifier, std::string payload = "") = 0;
		virtual void IterateAllMessages(std::function<void(const std::string&)> callback) = 0;
		virtual void IterateAllLogs(std::function<void(std::string)> callback) = 0;
	};
}