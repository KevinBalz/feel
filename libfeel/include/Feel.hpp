#pragma once
#include "Device.hpp"
#include "Finger.hpp"
#include "IncomingMessage.hpp"
#include "FeelStatus.hpp"
#include "CalibrationData.hpp"
#include <iomanip>
#include <map>
#include <array>
#include <cassert>
#include <functional>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream>
#include <limits>

namespace feel
{
    
    class Feel
    {
    public:       
        Feel(Device* device)
        {
            calibrationData.angles.fill(FingerCalibrationData{ 0, 180 });
            this->device = device;
        }

        ~Feel()
        {
            delete device;
        }

        void Connect(const char* deviceName)
        {
            device->Connect(deviceName);
            UpdateStatus();
        }

        void Disconnect()
        {
            device->Disconnect();
            UpdateStatus();
        }

        std::vector<std::string> GetAvailableDevices()
        {
            std::vector<std::string> devices;
            device->GetAvailableDevices(devices);
            return std::move(devices);
        }

        void GetAvailableDevices(std::vector<std::string>& devices)
        {
            device->GetAvailableDevices(devices);
        }

        void StartNormalization()
        {
            calibrationData.angles.fill(FingerCalibrationData{ std::numeric_limits<float>::max() , std::numeric_limits<float>::min() });
            device->TransmitMessage("IN");
            status = FeelStatus::Normalization;
        }

		void BeginSession()
        {
            device->TransmitMessage("BS");
            status = FeelStatus::Active;
        }

		void EndSession()
		{
			device->TransmitMessage("ES");
            status = FeelStatus::DeviceConnected;
            UpdateStatus();
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
				<< std::dec << std::setw(2)
                << force 
                << angle;
			device->TransmitMessage("WF", stream.str());
		}

        void ReleaseFinger(Finger finger)
        {
            std::stringstream stream;
            stream
                << std::setfill('0') << std::setw(2)
                << std::hex << static_cast<int>(finger);
            device->TransmitMessage("RE", stream.str());
        }

        void SetCalibrationData(CalibrationData& data)
        {
            calibrationData = data;
        }

		float GetFingerAngle(Finger finger) const
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

        FeelStatus GetStatus() const
        {
            return status;
        }

		void ParseMessages()
		{
            UpdateStatus();
			device->IterateAllMessages([&](auto message)
			{
				static std::map<std::string, IncomingMessage> incomingMessageMap =
				{
					{ "UF", IncomingMessage::FingerUpdate },
					{ "DL", IncomingMessage::DebugLog },
                    { "NI", IncomingMessage::NormalizationData },
                    { "EN", IncomingMessage::EndNormalization }
				};
				if (message.length() < 2) return;
				switch (incomingMessageMap[message.substr(0, 2)])
				{
					case IncomingMessage::DebugLog:
                    {
                        debugLogCallback(message.substr(2));
                    } break;
					case IncomingMessage::FingerUpdate:
                    {
                        std::string fingerIdentifier = message.substr(2, 2);
                        std::string fingerAngle = message.substr(4);
                        std::cout << "Finger: " << fingerIdentifier << " Angle: " << fingerAngle << std::endl;
                        int fingerIndex = std::stoul(fingerIdentifier, nullptr, 16);
                        fingerAngles[fingerIndex] = std::stof(fingerAngle);
                    } break;
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
                    } break;
                    case IncomingMessage::EndNormalization:
                    {
                        if (status == FeelStatus::Normalization)
                        {
                            status = FeelStatus::DeviceConnected;
                        }
                    } break;
				}
			});
		}
	private:
        FeelStatus status;
        Device* device = nullptr;
		std::function<void(std::string)> debugLogCallback = [](auto s)
        {
			std::cout << s << std::endl;
		};
		std::array<float, FINGER_TYPE_COUNT> fingerAngles = {0};
        CalibrationData calibrationData;

        void UpdateStatus()
        {
            switch (status)
            {
                case FeelStatus::DeviceDisconnected:
                case FeelStatus::DeviceConnecting:
                case FeelStatus::DeviceConnected:
                    switch (device->GetStatus())
                    {
                        case DeviceStatus::Disconnected:
                            status = FeelStatus::DeviceDisconnected;
                            break;
                        case DeviceStatus::Connecting:
                            status = FeelStatus::DeviceConnecting;
                            break;
                        case DeviceStatus::Connected:
                            status = FeelStatus::DeviceConnected;
                            break;
                    }
                    break;
            }
        }
	};
}