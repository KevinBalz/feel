#pragma once
#include "feel/Device.hpp"
#include "feel/Finger.hpp"
#include "feel/CalibrationData.hpp"
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <array>
#include <iomanip>
#include <string>
#include <sstream>
#include <atomic>

namespace feel
{
    class SimulatorDevice : public Device
    {
    public:

        SimulatorDevice() :
            status(DeviceStatus::Disconnected)
        {
        }

        ~SimulatorDevice()
        {
            if (messageGenerator.joinable())
            {
                threadFlag.clear();
                messageGenerator.join();
            }
        }

        DeviceStatus GetStatus() override
        {
            return status;
        }

        void Connect(const char* deviceName) override
        {
            status = DeviceStatus::Connecting;
            threadFlag.test_and_set();
            messageGenerator = std::thread(&SimulatorDevice::MessageGenerator, this);
            status = DeviceStatus::Connected;
        }

        void Disconnect()
        {
            status = DeviceStatus::Disconnected;
            threadFlag.clear();
            messageGenerator.join();
        }

        void GetAvailableDevices(std::vector<std::string>& devices)
        {
            devices.emplace_back("Simulator");
        }

        void TransmitMessage(std::string identifier, std::string payload = "") override
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            outputs.emplace(identifier + payload);
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

        void SetFingerPosition(Finger finger, int angle, int resistance)
        {
            int fingerIndex = static_cast<int>(finger);
            std::lock_guard<std::mutex> lock(fingerMutex);
            FingerPositionData& pos = fingerPositions[fingerIndex];
            pos.angle = angle;
            pos.resistance = resistance;
        }

    private:
        struct FingerOperationStatus
        {
            bool on = false;
            int targetAngle = 0;
            int targetForce = 0;
        };

        struct FingerPositionData
        {
            float angle = 0;
            int resistance = 0;
        };

        DeviceStatus status;
        std::queue<std::string> inputs;
        std::queue<std::string> outputs;
        std::thread messageGenerator;
        std::mutex inputMutex;
        std::mutex outputMutex;
        std::atomic_flag threadFlag;
        std::array<FingerPositionData, FINGER_TYPE_COUNT> fingerPositions;
        std::mutex fingerMutex;

        bool inNormalization = false;
        bool inSession = false;
        
        std::array<FingerOperationStatus, FINGER_TYPE_COUNT> fingerStatus;
        CalibrationData calibrationData;
        const int frameRate = 60;

        void MessageGenerator()
        {
            calibrationData.angles =
            {
                FingerCalibrationData{ 0, 180 },
                FingerCalibrationData{ 64, 400 },
                FingerCalibrationData{ 40, 270 },
                FingerCalibrationData{ 20, 109 },
                FingerCalibrationData{ 180, 337 },
                FingerCalibrationData{ 222, 542 },
                FingerCalibrationData{ 50, 390 },
                FingerCalibrationData{ 620, 820 },
                FingerCalibrationData{ 111, 424 },
                FingerCalibrationData{ 0, 111 }
            };
            while (threadFlag.test_and_set())
            {
                if (inNormalization)
                {
                    std::lock_guard<std::mutex> lock(inputMutex);
                    inputs.emplace("EN");
                    inNormalization = false;
                }
                ParseMessages();

                if (inSession)
                {
                    std::array<float, FINGER_TYPE_COUNT> angles;                
                    SimulateFingers(angles);
                    SendFingerUpdates(angles);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000/frameRate));
            }
        }

        void ResetFingerState()
        {
            FingerOperationStatus offStatus;
            offStatus.on = false;
            fingerStatus.fill(offStatus);
        }

        void ParseMessages()
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            while (!outputs.empty())
            {
                std::string& message = outputs.front();

                auto messageIdentifier = message.substr(0, 2);
                if (messageIdentifier == "IN")
                {
                    inNormalization = true;
                    inSession = false;

                    std::lock_guard<std::mutex> lock(inputMutex);
                    for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
                    {
                        auto data = calibrationData.angles[i];
                        for (int a = 0; a <= 180; a++)
                        {
                            std::stringstream stream;
                            stream
                                << "NI"
                                << std::setfill('0') << std::setw(2)
                                << std::hex << i
                                << std::dec << std::setw(3)
                                << a
                                << (int)std::round(a / 180.0f * (data.max - data.min) + data.min);
                            inputs.emplace(stream.str());
                        }
                    }
                }
                else if (messageIdentifier == "BS")
                {
                    ResetFingerState();
                    inSession = true;
                }
                else if (messageIdentifier == "ES")
                {
                    inSession = false;
                }
                else if (messageIdentifier == "WF")
                {
                    int fingerIndex = std::stoul(message.substr(2, 2), nullptr, 16);
                    FingerOperationStatus& status = fingerStatus[fingerIndex];
                    status.on = true;
                    status.targetForce = 99 - std::stoi(message.substr(4, 2));
                    status.targetAngle = std::stoi(message.substr(6));
                }
                else if (messageIdentifier == "RE")
                {
                    int fingerIndex = std::stoul(message.substr(2, 2), nullptr, 16);
                    FingerOperationStatus& status = fingerStatus[fingerIndex];
                    status.on = false;
                }
                else
                {
                    std::lock_guard<std::mutex> lock(inputMutex);
                    inputs.emplace("DLUnknown Message: " + message);
                }

                outputs.pop();
            }
        }

        void SimulateFingers(std::array<float, FINGER_TYPE_COUNT>& angles)
        {
            std::lock_guard<std::mutex> lock(fingerMutex);
            for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
            {
                FingerPositionData& pos = fingerPositions[i];
                FingerOperationStatus& status = fingerStatus[i];
                if (!status.on)
                {
                    angles[i] = pos.angle;
                    continue;
                }

                if ((int)std::round(pos.angle) == status.targetAngle)
                {
                    pos.angle = status.targetAngle;
                }
                else
                {
                    float forceFactor = std::max(0, std::max(status.targetForce, 1) - pos.resistance);
                    pos.angle += forceFactor / frameRate * (pos.angle - status.targetAngle > 0 ? -1 : 1);
                }

                angles[i] = pos.angle;
            }
        }

        void SendFingerUpdates(const std::array<float, FINGER_TYPE_COUNT>& angles)
        {
            std::lock_guard<std::mutex> lock(inputMutex);
            for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
            {
                const FingerCalibrationData& data = calibrationData.angles[i];
                std::stringstream stream;
                stream
                    << "UF"
                    << std::setfill('0') << std::setw(2)
                    << std::hex << i
                    << std::setw(3)
                    << std::dec << (int) std::round(angles[i] / 180 * (data.max - data.min) + data.min);
                inputs.emplace(stream.str());
            }
        }
    };
}