#pragma once
#include <array>

namespace feel
{
    struct FingerCalibrationData
    {
        float min;
        float max;
    };

    struct CalibrationData
    {
        std::array<FingerCalibrationData, FINGER_TYPE_COUNT> angles;
    };
}

