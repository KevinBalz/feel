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
#include <cmath>
#include <string>
#include <iostream>
#include <sstream>
#include <limits>

namespace feel
{
    struct FingerOperationStatus
    {
        bool on = false;
        int targetAngle = 0;
        int targetForce = 0;
    };


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

        /** Connects to the device by its name.
         *  Valid names are returned by GetAvailableDevices
         */
        void Connect(const char* deviceName)
        {
            device->Connect(deviceName);
            UpdateStatus();
        }

        /// Diconnect from the Device
        void Disconnect()
        {
            device->Disconnect();
            UpdateStatus();
        }

        /** Get a list of all names of available Devices
         *  which can be connected to.
         */
        std::vector<std::string> GetAvailableDevices()
        {
            std::vector<std::string> devices;
            device->GetAvailableDevices(devices);
            return std::move(devices);
        }

        /// Get a list of all names of available Devices which can be connected to.
        void GetAvailableDevices(std::vector<std::string>& devices)
        {
            device->GetAvailableDevices(devices);
        }

        /// Starts the Normalization Process
        /** Afterward BeginSession() can be called
         */
        void StartNormalization()
        {
            calibrationData.angles.fill(FingerCalibrationData{ std::numeric_limits<int>::max() , std::numeric_limits<int>::min() });
            device->TransmitMessage("IN");
            status = FeelStatus::Normalization;
        }

        /// Starts the session.
        /**  Ensure the device is calibrated using StartNormalization()
         *  or using SetCalibrationData(feel::CalibrationData&)
         */
		void BeginSession()
        {
            device->TransmitMessage("BS");
            for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
            {
                fingerAngles[i] = calibrationData.angles[i].min;
            }
            status = FeelStatus::Active;
        }

        /// Ends the session started by BeginSession()
		void EndSession()
		{
			device->TransmitMessage("ES");
            status = FeelStatus::DeviceConnected;
            UpdateStatus();
		}

        /// Move a finger to a specific angle
        /** @param finger The finger to move
         *  @param angle  The angle to move the finger to (0-180)
         *  @param force  How much force should be applied (0-99)
         */
		void SetFingerAngle(Finger finger, float angle, int force)
		{
			int fingerNumber = static_cast<int>(finger);
            FingerOperationStatus& status = fingerStatus[fingerNumber];
            force = 99 - force;
            int degree = (int)std::round(angle);
            if (status.on &&
                status.targetAngle == degree &&
                status.targetForce == force)
            {
                return;
            }
            auto data = calibrationData.angles[fingerNumber];
			std::stringstream stream;
			stream
				<< std::setfill('0') << std::setw(2)
				<< std::hex << fingerNumber
				<< std::dec << std::setw(2)
                << force
                << std::dec << std::setw(3)
                << degree;
			device->TransmitMessage("WF", stream.str());
            status.targetAngle = degree;
            status.targetForce = force;
            status.on = true;
		}

        void ReleaseFinger(Finger finger)
        {
            FingerOperationStatus& status = fingerStatus[static_cast<int>(finger)];
            if (!status.on) return;
            std::stringstream stream;
            stream
                << std::setfill('0') << std::setw(2)
                << std::hex << static_cast<int>(finger);
            device->TransmitMessage("RE", stream.str());
            status.on = false;
        }

        void SetCalibrationData(CalibrationData& data)
        {
            calibrationData = data;
        }

		float GetFingerAngle(Finger finger) const
		{
            auto angle = fingerAngles[static_cast<int>(finger)];
            auto data = calibrationData.angles[static_cast<int>(finger)];
            angle = (angle - data.min) / (data.max - data.min) * 180;
            return angle;
        }

        /** Set which function should be called when a Debug message from
         *  the device is processed
         */
		void SetDebugLogCallback(std::function<void(std::string) > callback)
		{
			debugLogCallback = callback;
		}

        FeelStatus GetStatus() const
        {
            return status;
        }

        /** Processes all incoming messages since the last call
         *  After calling it, all values are updated to the latest version.
         *  This should be called once per frame.
         */
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
                        int fingerIndex = std::stoul(fingerIdentifier, nullptr, 16);                  
                        fingerAngles[fingerIndex] = fingerAngles[fingerIndex] * 0.9f + std::stoi(fingerAngle) * 0.1f;
                    } break;
                    case IncomingMessage::NormalizationData:
                    {
                        std::string fingerIdentifier = message.substr(2, 2);
                        std::string realAngle = message.substr(4, 3);
                        std::string fingerAngle = message.substr(7);
                        debugLogCallback("Init Finger: " + fingerIdentifier + " Real Angle: " + realAngle + " Angle: " + fingerAngle);
                        int fingerIndex = std::stoul(fingerIdentifier, nullptr, 16);
                        int angle = std::stoi(fingerAngle);
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
        std::array<FingerOperationStatus, FINGER_TYPE_COUNT> fingerStatus;
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