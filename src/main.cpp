#define ASIO_STANDALONE
#include "Feel.hpp"

#include <chrono>
#include <iostream>


int main()
{
	feel::Feel feel;
	
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