#define FEEL_API __declspec(dllexport)
#include "Feel.hpp"

extern "C"
{
	FEEL_API feel::Feel* FEEL_CreateNew()
	{
		return new feel::Feel();
	}

	FEEL_API void FEEL_ParseMessages(feel::Feel* feel)
	{
		feel->ParseMessages();
	}
}