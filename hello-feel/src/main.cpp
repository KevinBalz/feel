#include "Feel.hpp"

#include <chrono>
#include <iostream>


int main()
{
	feel::Feel feel;

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
		feel.SetFingerAngle(feel::Finger::HAND_0_RING_1, 123.4f);
		feel.SetFingerAngle(feel::Finger::HAND_0_RING_0, 999);
		feel.ParseMessages();

		for (int i = 0; i < feel::FINGER_TYPE_COUNT; i++)
		{
			std::cout << feel.GetFingerAngle(static_cast<feel::Finger>(i)) << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	feel.EndSession();

    return 0;
}