// Copyright 2015-2018 Elviss Strazdins. All rights reserved.

#pragma once

#include "input/InputDevice.hpp"

namespace ouzel
{
    namespace input
    {
        class KeyboardDeviceTVOS: public InputDevice
        {
        public:
            KeyboardDeviceTVOS(InputSystem& initInputSystem,
                               uint32_t initId):
                InputDevice(initInputSystem, initId)
            {
            }

            virtual ~KeyboardDeviceTVOS() {}
        };
    } // namespace input
} // namespace ouzel
