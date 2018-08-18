#pragma once
#include "feel/Device.hpp"
#include "feel/Finger.hpp"
#include "feel/IncomingMessage.hpp"
#include "feel/FeelStatus.hpp"
#include "feel/CalibrationData.hpp"
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
    class Feel
    {
    public:

        /// @brief Creates a new instance
        /// @param device The underlying device to use.
        /// @note The given device will be deleted in the destructor.       
        Feel(Device* device)
        {
            calibrationData.angles.fill(FingerCalibrationData{ 0, 180 });
            this->device = device;
        }

        ~Feel()
        {
            delete device;
        }

        /// @brief Start to connect to the device.
        /// @param deviceName The name of the device to connect to. (A name returned by GetAvailableDevices() )
        void Connect(const char* deviceName)
        {
            device->Connect(deviceName);
            UpdateStatus();
        }

        /// @brief Diconnect from the Device
        void Disconnect()
        {
            device->Disconnect();
            UpdateStatus();
        }

        //@{
        /// @brief Get a list of all names of available Devices
        ///
        /// Every name can be used to connect via Connect()

        /// @return A Vector with all names
        std::vector<std::string> GetAvailableDevices()
        {
            std::vector<std::string> devices;
            device->GetAvailableDevices(devices);
            return std::move(devices);
        }

        /// @post all names have been added to the vector
        void GetAvailableDevices(std::vector<std::string>& devices)
        {
            device->GetAvailableDevices(devices);
        }
        //@}

        /// @brief Starts the Normalization Process
        ///
        /// Afterwards BeginSession() can be called
        void StartNormalization()
        {
            calibrationData.angles.fill(FingerCalibrationData{ std::numeric_limits<int>::max() , std::numeric_limits<int>::min() });
            device->TransmitMessage("IN");
            status = FeelStatus::Normalization;
        }

        /// @brief Set the normalization data.
        ///
        /// This can be used to skip StartNormalization() 
        /// when used with data from a previous session.
        /// @param data The normalization data.
        void SetCalibrationData(CalibrationData& data)
        {
            calibrationData = data;
        }

        /// @brief Starts the session.
        ///
        /// Ensure the device is calibrated using StartNormalization()
        /// or using SetCalibrationData(feel::CalibrationData&),
        /// otherwise angles returned from GetFingerAngle() might not be correct
		void BeginSession()
        {
            device->TransmitMessage("BS");
            for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
            {
                fingerAngles[i] = calibrationData.angles[i].min;
            }
            status = FeelStatus::Active;
        }

        /// @brief Ends the session started by BeginSession()
		void EndSession()
		{
			device->TransmitMessage("ES");
            status = FeelStatus::DeviceConnected;
            UpdateStatus();
		}

        /// @brief Move a finger to a specific angle
        /// @param finger The finger to move
        /// @param angle  The angle to move the finger to (0-180)
        /// @param force  How much force should be applied (0-99)
        /// \note The force may not be applied immediately.
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

        /// @brief Release the force from a finger.
        ///
        /// This makes the finger be able to move freely
        /// @param finger The finger to release
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

        /// @brief Get the angle a finger is at.
        ///
        /// @param finger The finger to get the angle from.
        /// @return The angle the finger is at, ranges from 0 - 180.
		float GetFingerAngle(Finger finger) const
		{
            auto angle = fingerAngles[static_cast<int>(finger)];
            auto data = calibrationData.angles[static_cast<int>(finger)];
            angle = (angle - data.min) / (data.max - data.min) * 180;
            return angle;
        }

        /// @brief Set which function should be called when a Debug message from
        /// the device is processed
		void SetDebugLogCallback(std::function<void(std::string) > callback)
		{
			debugLogCallback = callback;
		}

        /// @brief Get the current feel::FeelStatus
        /// @return The current status
        FeelStatus GetStatus() const
        {
            return status;
        }

        /// @brief Processes all incoming messages since the last call
        ///
        /// After calling it, all values are updated to the latest version.
        /// This function should typically be called once per frame.
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
        struct FingerOperationStatus
        {
            bool on = false;
            int targetAngle = 0;
            int targetForce = 0;
        };

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