// Copyright 2015-2019 Elviss Strazdins. All rights reserved.

#include <stdexcept>
#include "core/Setup.h"
#if OUZEL_SUPPORTS_X11
#  include <X11/cursorfont.h>
#  include <X11/Xcursor/Xcursor.h>
#endif
#include "CursorLinux.hpp"
#include "core/linux/EngineLinux.hpp"

namespace ouzel
{
    namespace input
    {
        CursorLinux::CursorLinux(SystemCursor systemCursor)
        {
#if OUZEL_SUPPORTS_X11
            EngineLinux* engineLinux = static_cast<EngineLinux*>(engine);
            Display* display = engineLinux->getDisplay();

            switch (systemCursor)
            {
                case SystemCursor::Default:
                case SystemCursor::Arrow:
                    cursor = XcursorLibraryLoadCursor(display, "arrow");
                    break;
                case SystemCursor::Hand:
                    cursor = XcursorLibraryLoadCursor(display, "hand1");
                    break;
                case SystemCursor::HorizontalResize:
                    cursor = XcursorLibraryLoadCursor(display, "sb_h_double_arrow");
                    break;
                case SystemCursor::VerticalResize:
                    cursor = XcursorLibraryLoadCursor(display, "sb_v_double_arrow");
                    break;
                case SystemCursor::Cross:
                    cursor = XcursorLibraryLoadCursor(display, "crosshair");
                    break;
                case SystemCursor::IBeam:
                    cursor = XcursorLibraryLoadCursor(display, "xterm");
                    break;
            }
#endif
        }

        CursorLinux::CursorLinux(const std::vector<uint8_t>& data,
                                 const Size2F& size,
                                 graphics::PixelFormat pixelFormat,
                                 const Vector2F& hotSpot)
        {
#if OUZEL_SUPPORTS_X11
            EngineLinux* engineLinux = static_cast<EngineLinux*>(engine);
            Display* display = engineLinux->getDisplay();

            if (!data.empty())
            {
                auto width = static_cast<int>(size.v[0]);
                auto height = static_cast<int>(size.v[1]);

                XcursorImage* cursorImage = XcursorImageCreate(width, height);

                if (!cursorImage)
                    throw std::runtime_error("Failed to create cursor image");

                cursorImage->xhot = static_cast<int>(hotSpot.v[0]);
                cursorImage->yhot = height - static_cast<int>(hotSpot.v[1]) - 1;
                cursorImage->delay = 0;

                for (int i = 0; i < width * height; ++i)
                {
                    // premultiply alpha
                    const float opacity = data[i * 4 + 3] / 255.0F;

                    cursorImage->pixels[i] = static_cast<unsigned int>(data[i * 4 + 2] * opacity) |
                        (static_cast<unsigned int>(data[i * 4 + 1] * opacity) << 8) |
                        (static_cast<unsigned int>(data[i * 4 + 0] * opacity) << 16) |
                        (data[i * 4 + 3] << 24);
                }

                cursor = XcursorImageLoadCursor(display, cursorImage);

                XcursorImageDestroy(cursorImage);
            }
#endif
        }

        CursorLinux::~CursorLinux()
        {
#if OUZEL_SUPPORTS_X11
            EngineLinux* engineLinux = static_cast<EngineLinux*>(engine);
            Display* display = engineLinux->getDisplay();
            if (cursor != None) XFreeCursor(display, cursor);
#endif
        }
    } // namespace input
} // namespace ouzel
