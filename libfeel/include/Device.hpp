#pragma once
#include <string>
#include <functional>
#include <vector>

namespace feel
{
	class Device
	{
	public:
		virtual void Connect(const char* deviceName) = 0;
        virtual void GetAvailableDevices(std::vector<std::string>& devices) = 0;
		virtual void TransmitMessage(std::string identifier, std::string payload = "") = 0;
		virtual void IterateAllMessages(std::function<void(std::string)> callback) = 0;
		virtual void IterateAllLogs(std::function<void(std::string)> callback) = 0;
	};
}