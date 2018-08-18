#ifdef _WIN32
#define FEEL_API __declspec(dllexport)
#else
#define FEEL_API
#endif
#include "feel.hpp"
#include <memory>

extern "C"
{
    typedef intptr_t FeelStringArrayHandle;
    typedef feel::Feel<std::unique_ptr<feel::Device> > FeelHandle;
    struct FeelStringArray
    {
        char** strings;
        int size;
    };

    FEEL_API FeelHandle* FEEL_CreateNewWithDevice(feel::Device* device)
    {
        return new FeelHandle(std::unique_ptr<feel::Device>(device));
    }

	FEEL_API FeelHandle* FEEL_CreateWithSerialDevice()
	{
        return new FeelHandle(std::make_unique<feel::SerialDevice>());
	}

    FEEL_API FeelHandle* FEEL_CreateWithSimulatorDevice()
    {
        return new FeelHandle(std::make_unique<feel::SimulatorDevice>());
    }

	FEEL_API void FEEL_Connect(FeelHandle* feel, const char* deviceName)
    {
        feel->Connect(deviceName);
    }

    FEEL_API void FEEL_Disconnect(FeelHandle* feel)
    {
        feel->Disconnect();
    }

    FEEL_API void FEEL_GetAvailableDevices(FeelHandle* feel, FeelStringArrayHandle* handle, char*** devices, int* deviceCount)
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

    FEEL_API void FEEL_StartNormalization(FeelHandle* feel)
    {
        feel->StartNormalization();
    }

    FEEL_API void FEEL_BeginSession(FeelHandle* feel)
    {
        feel->BeginSession();
    }

	FEEL_API void FEEL_Destroy(FeelHandle* feel)
    {
        delete feel;
    }

    FEEL_API void FEEL_EndSession(FeelHandle* feel)
    {
        feel->EndSession();
    }

    FEEL_API void FEEL_SetFingerAngle(FeelHandle* feel, int finger, float angle, int force)
    {
        feel->SetFingerAngle(static_cast<feel::Finger>(finger), angle, force);
    }

    FEEL_API void FEEL_ReleaseFinger(FeelHandle* feel, int finger)
    {
        return feel->ReleaseFinger(static_cast<feel::Finger>(finger));
    }

    FEEL_API float FEEL_GetFingerAngle(FeelHandle* feel, int finger)
    {
        return feel->GetFingerAngle(static_cast<feel::Finger>(finger));
    }

    FEEL_API int FEEL_GetStatus(FeelHandle* feel)
    {
        return feel->GetStatus();
    }

	FEEL_API void FEEL_ParseMessages(FeelHandle* feel)
	{
		feel->ParseMessages();
	}

    FEEL_API void FEEL_SetDebugLogCallback(FeelHandle* feel, void (*callback)(const char*))
    {
        feel->SetDebugLogCallback([callback](std::string s)
        {
            callback(s.c_str());
        });
    }
}