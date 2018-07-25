#include "Feel.hpp"

#include <chrono>
#include <iostream>
#include "SimulatorDevice.hpp"


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
	feel.BeginSession();
	feel.SubscribeForFingerUpdates();

	for (;;)
	{
		std::cout << "PP" << std::endl;
		feel.SetFingerAngle(feel::Finger::HAND_0_RING_1, 9, 33);
		feel.SetFingerAngle(feel::Finger::HAND_0_RING_0, 137, 99);
		feel.ParseMessages();

		for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
		{
			std::cout << feel.GetFingerAngle(static_cast<feel::Finger>(i)) << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	feel.EndSession();
    feel.Disconnect();

    return 0;
}