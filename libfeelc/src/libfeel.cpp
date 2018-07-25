#ifdef _WIN32
#define FEEL_API __declspec(dllexport)
#else
#define FEEL_API
#endif
#include "Feel.hpp"
#include "Finger.hpp"
#include <memory>

extern "C"
{
    typedef intptr_t FeelStringArrayHandle;
    struct FeelStringArray
    {
        char** strings;
        int size;
    };

	FEEL_API feel::Feel* FEEL_CreateNew()
	{
		return new feel::Feel();
	}

	FEEL_API void FEEL_Connect(feel::Feel* feel, const char* deviceName)
    {
        feel->Connect(deviceName);
    }

    FEEL_API void FEEL_Disconnect(feel::Feel* feel)
    {
        feel->Disconnect();
    }

    FEEL_API void FEEL_GetAvailableDevices(feel::Feel* feel, FeelStringArrayHandle* handle, char*** devices, int* deviceCount)
    {
        FeelStringArray* arr = new FeelStringArray();
        auto deviceNames = feel->GetAvailableDevices();
        arr->size = deviceNames.size();
        arr->strings = new char*[arr->size];
        for (int i = 0; i < arr->size; i++)
        {
            arr->strings[i] = new char[deviceNames[i].length() + 1];
            strcpy(arr->strings[i], deviceNames[i].c_str());
        }

        *handle = reinterpret_cast<FeelStringArrayHandle>(arr);
        *devices = arr->strings;
        *deviceCount = arr->size;
    }

    FEEL_API void FEEL_ReleaseFeelStringArrayHandle(FeelStringArrayHandle* handle)
    {
        auto arr = reinterpret_cast<FeelStringArray*>(handle);
        for (int i = 0; i < arr->size; i++)
        {
            delete arr->strings[i];
        }
        delete arr;
    }

    FEEL_API void FEEL_StartNormalization(feel::Feel* feel)
    {
        feel->StartNormalization();
    }

    FEEL_API void FEEL_BeginSession(feel::Feel* feel)
    {
        feel->BeginSession();
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

    FEEL_API void FEEL_SetFingerAngle(feel::Feel* feel, int finger, float angle, int force)
    {
        feel->SetFingerAngle(static_cast<feel::Finger>(finger), angle, force);
    }

    FEEL_API float FEEL_GetFingerAngle(feel::Feel* feel, int finger)
    {
        return feel->GetFingerAngle(static_cast<feel::Finger>(finger));
    }

	FEEL_API void FEEL_ParseMessages(feel::Feel* feel)
	{
		feel->ParseMessages();
	}

    FEEL_API void FEEL_SetDebugLogCallback(feel::Feel* feel, void (*callback)(const char*))
    {
        feel->SetDebugLogCallback([callback](std::string s)
        {
            callback(s.c_str());
        });
    }
}