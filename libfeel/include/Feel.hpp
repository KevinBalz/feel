#pragma once
#include "SerialDevice.hpp"
#include "Finger.hpp"
#include "IncomingMessage.hpp"
#include <iomanip>
#include <map>
#include <array>
#include <cassert>
#include <functional>
#include <algorithm>
#include <string>
#include <limits>

namespace feel
{
    struct FingerCalibrationData
    {
        float min;
        float max;
    };

    struct CalibrationData
    {
        std::array<FingerCalibrationData, FINGER_TYPE_COUNT> angles;
    };
    
    class Feel
    {
    public:
        Feel()
        {
            calibrationData.angles.fill(FingerCalibrationData{0, 180});
        }

        void Connect(const char* deviceName)
        {
            device.Connect(deviceName);
        }

        void Disconnect()
        {
            device.Disconnect();
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

        void StartNormalization()
        {
            calibrationData.angles.fill(FingerCalibrationData{ std::numeric_limits<float>::max() , std::numeric_limits<float>::min() });
            device.TransmitMessage("IN");
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

		void SetFingerAngle(Finger finger, float angle, int force)
		{
			int fingerNumber = static_cast<int>(finger);
            auto data = calibrationData.angles[fingerNumber];
            angle = angle / 180 * data.max + data.min;
            force = 99 - force;
			std::stringstream stream;
			stream
				<< std::setfill('0') << std::setw(2)
				<< std::hex << fingerNumber
				<< std::dec << force << angle;
			device.TransmitMessage("WF", stream.str());
		}

        void SetCalibrationData(CalibrationData& data)
        {
            calibrationData = data;
        }

		float GetFingerAngle(Finger finger)
		{
            auto angle = fingerAngles[static_cast<int>(finger)];
            auto data = calibrationData.angles[static_cast<int>(finger)];
            angle = (angle - data.min) / data.max * 180;
            return angle;
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
					{ "DL", IncomingMessage::DebugLog },
                    { "NI", IncomingMessage::NormalizationData }
				};
				if (message.length() < 2) return;
				switch (incomingMessageMap[message.substr(0, 2)])
				{
					case IncomingMessage::DebugLog:
						debugLogCallback(message.substr(2));
						break;
					case IncomingMessage::FingerUpdate:
                    {
                        std::string fingerIdentifier = message.substr(2, 2);
                        std::string fingerAngle = message.substr(4);
                        std::cout << "Finger: " << fingerIdentifier << " Angle: " << fingerAngle << std::endl;
                        int fingerIndex = std::stoul(fingerIdentifier, nullptr, 16);
                        fingerAngles[fingerIndex] = std::stof(fingerAngle);
                    }
                    break;
                    case IncomingMessage::NormalizationData:
                    {
                        std::string fingerIdentifier = message.substr(2, 2);
                        std::string realAngle = message.substr(4, 3);
                        std::string fingerAngle = message.substr(7);
                        std::cout << "Init Finger: " << fingerIdentifier << " Real Angle: " << realAngle << " Angle: " << fingerAngle << std::endl;
                        int fingerIndex = std::stoul(fingerIdentifier, nullptr, 16);
                        float angle = std::stof(fingerAngle);
                        auto data = calibrationData.angles[fingerIndex];
                        data.min = std::min(data.min, angle);
                        data.max = std::max(data.max, angle);
                        calibrationData.angles[fingerIndex] = data;
                    }
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
        CalibrationData calibrationData;
	};
}