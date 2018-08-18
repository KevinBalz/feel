#include "feel.hpp"
#include <chrono>
#include <iostream>
#include <atomic>
#include <Windows.h>
#include <memory>


static std::atomic_flag keepRunning;
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
            keepRunning.clear();
            return TRUE;
        default: return FALSE;
    }
}

int main()
{
	auto feel = feel::Create(std::move(std::make_unique<feel::SerialDevice>()));
    //feel::Feel feel(new feel::SimulatorDevice());

    auto devices = feel.GetAvailableDevices();
    if (devices.empty())
    {
        std::cout << "No device found" << std::endl;
        std::string s;
        std::cin >> s;
        return 0;
    }
    for (auto device : devices)
    {
        std::cout << device << std::endl;
    }
    
	feel.Connect(devices[0].c_str());

    keepRunning.test_and_set();
    SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE);

    feel.StartNormalization();
    while (feel.GetStatus() == feel::FeelStatus::Normalization)
    {
        if (!keepRunning.test_and_set())
        {
            feel.Disconnect();
            return 0;
        }
        std::cout << "Normalization Frame" << std::endl;
        feel.ParseMessages();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

	feel.BeginSession();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    feel.ParseMessages();
    std::array<bool, feel::FINGER_TYPE_COUNT> fingerBelow;
    std::array<std::array<float, 10>, feel::FINGER_TYPE_COUNT> lastPositions;

    for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
    {
        feel::Finger finger = static_cast<feel::Finger>(i);
        lastPositions[i].fill(feel.GetFingerAngle(finger));
    }

	while (keepRunning.test_and_set())
	{
		std::cout << "Process Frame" << std::endl;
        feel.ParseMessages();

        for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
        {
            feel::Finger finger = static_cast<feel::Finger>(i);
            float angle = feel.GetFingerAngle(finger);
            float velocity = (angle - lastPositions[i][0]) * 10 * 0.016f;
            std::cout << "Finger " << i << ": " << angle
                << " Velocity " << velocity;

            const float midAngle = 60;
            bool released = false;
            float force;
            if (fingerBelow[i] && angle > midAngle - 5 && angle + 5 < midAngle && velocity > 0)
            {
                feel.ReleaseFinger(finger);
                released = true;
            }
            else if (angle < midAngle - 10 && velocity < -1)
            {
                force = 99;
                feel.SetFingerAngle(finger, 180, force);
                fingerBelow[i] = true;
            }
            else if (angle < midAngle && std::abs(velocity) > 0.5f)
            {
                force = 40;
                feel.SetFingerAngle(finger, midAngle + (midAngle - angle), force);
                fingerBelow[i] = true;
            }
            else
            {
                feel.ReleaseFinger(finger);
                fingerBelow[i] = false;
                released = true;
            }

            if (!released) std::cout << " Force " << force;
            std::cout << std::endl;

            std::rotate(lastPositions[i].begin(), lastPositions[i].begin() + 1, lastPositions[i].end());
            lastPositions[i][9] = angle;

        }

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	feel.EndSession();
    feel.Disconnect();

    return 0;
}