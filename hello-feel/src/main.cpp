#include "Feel.hpp"

#include <chrono>
#include <iostream>
#include "SimulatorDevice.hpp"
#include <Windows.h>


static bool shouldTerminate;
BOOL ConsoleCtrlHandler(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
            shouldTerminate = true;
            return TRUE;
        default: return FALSE;
    }
}

int main()
{
	feel::Feel feel(new feel::SimulatorDevice());

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

    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    feel.StartNormalization();
    while (feel.GetStatus() == feel::FeelStatus::Normalization && !shouldTerminate)
    {
        std::cout << "Normalization Frame" << std::endl;
        feel.ParseMessages();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (shouldTerminate)
    {
        feel.Disconnect();
        return 0;
    }

	feel.BeginSession();

	while (!shouldTerminate)
	{
		std::cout << "Process Frame" << std::endl;
        feel.ParseMessages();

        for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
        {
            feel::Finger finger = static_cast<feel::Finger>(i);
            float angle = feel.GetFingerAngle(finger);
            std::cout << "Finger " << i << ": " <<  angle << std::endl;

            if (angle >= 90)
            {
                float force = (angle - 90) / 90 * 99;
                feel.SetFingerAngle(finger, 90, force);
            }
            else
            {
                feel.ReleaseFinger(finger);
            }
        }

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	feel.EndSession();
    feel.Disconnect();

    return 0;
}