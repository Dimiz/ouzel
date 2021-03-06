// Copyright 2015-2019 Elviss Strazdins. All rights reserved.

#include "CursorMacOS.hpp"

namespace ouzel
{
    namespace input
    {
        CursorMacOS::CursorMacOS(SystemCursor systemCursor)
        {
            switch (systemCursor)
            {
                case SystemCursor::Default:
                case SystemCursor::Arrow:
                    cursor = [[NSCursor arrowCursor] retain];
                    break;
                case SystemCursor::Hand:
                    cursor = [[NSCursor openHandCursor] retain];
                    break;
                case SystemCursor::HorizontalResize:
                    cursor = [[NSCursor resizeLeftRightCursor] retain];
                    break;
                case SystemCursor::VerticalResize:
                    cursor = [[NSCursor resizeUpDownCursor] retain];
                    break;
                case SystemCursor::Cross:
                    cursor = [[NSCursor crosshairCursor] retain];
                    break;
                case SystemCursor::IBeam:
                    cursor = [[NSCursor IBeamCursor] retain];
                    break;
            }
        }

        CursorMacOS::CursorMacOS(const std::vector<uint8_t>& newData,
                                 const Size2F& size,
                                 graphics::PixelFormat pixelFormat,
                                 const Vector2F& hotSpot):
            data(newData)
        {
            if (!data.empty())
            {
                const NSInteger bytesPerPixel = graphics::getPixelSize(pixelFormat);
                const NSInteger channelSize = graphics::getChannelSize(pixelFormat);
                const NSInteger channelCount = graphics::getChannelCount(pixelFormat);
                const auto width = static_cast<NSInteger>(size.v[0]);
                const auto height = static_cast<NSInteger>(size.v[1]);

                unsigned char* rgba = data.data();

                NSImage* image = [[[NSImage alloc]
                                   initWithSize:NSMakeSize(width, height)] autorelease];
                NSBitmapImageRep* imageRep = [[[NSBitmapImageRep alloc]
                                               initWithBitmapDataPlanes:&rgba
                                               pixelsWide:width
                                               pixelsHigh:height
                                               bitsPerSample:channelSize * 8
                                               samplesPerPixel:channelCount
                                               hasAlpha:YES
                                               isPlanar:NO
                                               colorSpaceName:NSDeviceRGBColorSpace
                                               bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                                               bytesPerRow:width * bytesPerPixel
                                               bitsPerPixel:bytesPerPixel * 8] autorelease];

                [image addRepresentation:imageRep];
                cursor = [[NSCursor alloc] initWithImage:image
                                                 hotSpot:NSMakePoint(hotSpot.v[0], size.v[1] - hotSpot.v[1] - 1.0F)];
            }
        }

        CursorMacOS::~CursorMacOS()
        {
            if (cursor) [cursor release];
        }
    } // namespace input
} // namespace ouzel
