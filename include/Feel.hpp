#include "SerialConnection.hpp"
#include "Finger.hpp"
#include "IncomingMessage.hpp"
#include <iomanip>
#include <map>
#pragma once
#include <cassert>

namespace feel
{
	class Feel
	{
	public:
		Feel()
		{
			connection.SendSerialMessage("BS");
		}

		void EndSession()
		{
			connection.SendSerialMessage("ES");
		}

		void SubscribeForFingerUpdates(bool active = true)
		{
			connection.SendSerialMessage("SS", active ? "1" : "0");
		}

		void SetFingerAngle(Finger finger, float angle)
		{
			int fingerNumber = static_cast<int>(finger);
			std::stringstream stream;
			stream
				<< std::setfill('0') << std::setw(2)
				<< std::hex << fingerNumber
				<< std::dec << angle;
			connection.SendSerialMessage("WF", stream.str());
		}

		void ParseMessages()
		{
			connection.IterateAllMessages([](auto message)
			{
				std::map<std::string, IncomingMessage> incomingMessageMap =
				{
					{ "UF", IncomingMessage::FingerUpdate },
					{ "DL", IncomingMessage::DebugLog }
				};
				if (message.length() < 2) return;
				switch (incomingMessageMap[message.substr(0, 2)])
				{
					case IncomingMessage::DebugLog:
						std::cout << "DebugLog: " << message.substr(2) << std::endl;
						break;
					case IncomingMessage::FingerUpdate:
						std::cout << "Finger: " << message.substr(2, 2) << " Angle: " << message.substr(4) << std::endl;
				}
			});
		}
	private:
		SerialConnection connection;
	};
}