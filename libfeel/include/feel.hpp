#pragma once

#include "feel/Feel.hpp"
#include "feel/SerialDevice.hpp"
#include "feel/SimulatorDevice.hpp"

namespace feel
{
    template<typename DevicePtr>
    Feel<DevicePtr> Create(DevicePtr devicePtr)
    {
        return std::move(Feel<DevicePtr>(devicePtr));
    }
}