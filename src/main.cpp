#define ASIO_STANDALONE
#include "Finger.hpp"
#include "SerialConnection.hpp"

#include <chrono>
#include <iostream>


int main()
{
	SerialConnection con;
	con.SubscribeForFingerUpdates();
	
	for (;;)
	{
		std::cout << "PP" << std::endl;
		con.PrintAllMessages();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		
	}

    return 0;
}