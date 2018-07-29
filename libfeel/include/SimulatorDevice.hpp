#pragma once
#include "Device.hpp"
#include "Finger.hpp"
#include "CalibrationData.hpp"
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

        void IterateAllLogs(std::function<void(std::string)> callback) override
        {}
    private:
        DeviceStatus status;
        std::queue<std::string> inputs;
        std::queue<std::string> outputs;
        std::thread messageGenerator;
        std::mutex inputMutex;
        std::mutex outputMutex;
        std::atomic_flag threadFlag;

        void MessageGenerator()
        {
            bool inNormalization = false;
            bool inSession = false;
            std::array<float, FINGER_TYPE_COUNT> angles = { 0 };
            std::array<float, FINGER_TYPE_COUNT> targetAngles = { 0 };
            CalibrationData calibrationData;
            calibrationData.angles =
            {
                FingerCalibrationData{ 32, 200 },
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
                                        << a / 180.0f * data.max + data.min;
                                    inputs.emplace(stream.str());
                                }
                                angles[i] = data.min;
                            }
                        }
                        else if (messageIdentifier == "BS")
                        {
                            inSession = true;
                        }
                        else if (messageIdentifier == "ES")
                        {
                            inSession = false;
                        }
                        else if (messageIdentifier == "WF")
                        {
                            int fingerIndex = std::stoul(message.substr(2, 2) , nullptr, 16);
                            angles[fingerIndex] = std::stof(message.substr(6));
                        }
                        else
                        {
                            std::lock_guard<std::mutex> lock(inputMutex);
                            inputs.emplace("DLUnknown Message: " + message);
                        }

                        outputs.pop();
                    }
                }

                if (inSession)
                {
                    std::lock_guard<std::mutex> lock(inputMutex);
                    for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
                    {
                        auto data = calibrationData.angles[i];
                        std::stringstream stream;
                        stream
                            << "UF"
                            << std::setfill('0') << std::setw(2)
                            << std::hex << i
                            << std::dec << angles[i];
                        inputs.emplace(stream.str());
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
            }
        }
    };
}