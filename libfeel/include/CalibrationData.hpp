#pragma once
#include <array>

namespace feel
{
    struct FingerCalibrationData
    {
        int min;
        int max;
    };

    struct CalibrationData
    {
        std::array<FingerCalibrationData, FINGER_TYPE_COUNT> angles;
    };
}

