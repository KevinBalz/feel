#ifdef _WIN32
#define FEEL_API __declspec(dllexport)
#else
#define FEEL_API
#endif
#include "Feel.hpp"
#include "Finger.hpp"

extern "C"
{
	FEEL_API feel::Feel* FEEL_CreateNew()
	{
		return new feel::Feel();
	}

	FEEL_API void FEEL_Destroy(feel::Feel* feel)
    {
        delete feel;
    }

    FEEL_API void FEEL_EndSession(feel::Feel* feel)
    {
        feel->EndSession();
    }

    FEEL_API void FEEL_SubscribeForFingerUpdates(feel::Feel* feel, bool active)
    {
        feel->SubscribeForFingerUpdates(active);
    }

    FEEL_API void FEEL_SetFingerAngle(feel::Feel* feel, int finger, float angle)
    {
        feel->SetFingerAngle(static_cast<feel::Finger>(finger), angle);
    }

    FEEL_API float FEEL_GetFingerAngle(feel::Feel* feel, int finger)
    {
        return feel->GetFingerAngle(static_cast<feel::Finger>(finger));
    }

	FEEL_API void FEEL_ParseMessages(feel::Feel* feel)
	{
		feel->ParseMessages();
	}

    FEEL_API void FEEL_SetDebugLogCallback(feel::Feel* feel, void (*callback)(char*, ))
    {
        feel->SetDebugLogCallback([callback](auto s)
        {
            callback(s.c_str());
        });
    }
}