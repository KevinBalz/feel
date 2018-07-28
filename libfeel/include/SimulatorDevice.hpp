#pragma once

#include "Device.hpp"
#include "Finger.hpp"
#include "CalibrationData.hpp"
#include <thread>
#include <queue>
#include <chrono>
#include <array>
#include <iomanip>
#include <string>

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
            messageGenerator = std::thread(MessageGenerator, std::ref(*this));
            status = DeviceStatus::Connected;
        }

        void Disconnect()
        {
            status = DeviceStatus::Disconnected;
            messageGenerator.join();
        }

        void GetAvailableDevices(std::vector<std::string>& devices)
        {
            devices.emplace_back("Simulator");
        }

        void TransmitMessage(std::string identifier, std::string payload = "") override
        {
            auto str = identifier + payload;
            std::lock_guard<std::mutex> lock(outputMutex);
            outputs.push(str);
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

        static void MessageGenerator(SimulatorDevice& simulator)
        {
            bool inNormalization = false;
            bool inSession = false;
            bool subscribed = false;
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
            while (simulator.status != DeviceStatus::Disconnected)
            {
                if (inNormalization)
                {
                    std::lock_guard<std::mutex> lock(simulator.inputMutex);
                    simulator.inputs.push("EN");
                    inNormalization = false;
                }
                {
                    std::lock_guard<std::mutex> lock(simulator.outputMutex);
                    while (!simulator.outputs.empty())
                    {
                        std::string& message = simulator.outputs.front();

                        auto messageIdentifier = message.substr(0, 2);
                        if (messageIdentifier == "IN")
                        {
                            inNormalization = true;
                            inSession = false;
                            subscribed = false;

                            std::lock_guard<std::mutex> lock(simulator.inputMutex);
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
                                    simulator.inputs.push(stream.str());
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
                            subscribed = false;
                        }
                        else if (messageIdentifier == "SF")
                        {
                            subscribed = message.substr(2, 1) == "1";
                        }
                        else if (messageIdentifier == "WF")
                        {
                            int fingerIndex = std::stoul(message.substr(2, 2) , nullptr, 16);
                            angles[fingerIndex] = std::stof(message.substr(6));
                        }
                        else
                        {
                            std::lock_guard<std::mutex> lock(simulator.inputMutex);
                            simulator.inputs.push("DLUnknown Message: " + message);
                        }

                        simulator.outputs.pop();
                    }
                }

                if (subscribed)
                {
                    std::lock_guard<std::mutex> lock(simulator.inputMutex);
                    for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
                    {
                        auto data = calibrationData.angles[i];
                        std::stringstream stream;
                        stream
                            << "UF"
                            << std::setfill('0') << std::setw(2)
                            << std::hex << i
                            << std::dec << angles[i];
                        simulator.inputs.push(stream.str());
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
            }
        }
    };
}