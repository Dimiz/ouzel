// Copyright 2015-2019 Elviss Strazdins. All rights reserved.

#include "core/Setup.h"

#if defined(__APPLE__)
#  include <TargetConditionals.h>
#endif

#if TARGET_OS_MAC && !TARGET_OS_IOS && !TARGET_OS_TV && OUZEL_COMPILE_OPENGL

#include "OGLRenderDeviceMacOS.hpp"
#include "OpenGLView.h"
#include "core/macos/NativeWindowMacOS.hpp"
#include "core/Engine.hpp"
#include "utils/Log.hpp"

static CVReturn renderCallback(CVDisplayLinkRef,
                               const CVTimeStamp*,
                               const CVTimeStamp*,
                               CVOptionFlags,
                               CVOptionFlags*,
                               void* userInfo)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    try
    {
        ouzel::graphics::opengl::RenderDeviceMacOS* renderDevice = static_cast<ouzel::graphics::opengl::RenderDeviceMacOS*>(userInfo);
        renderDevice->renderCallback();
    }
    catch (const std::exception& e)
    {
        ouzel::engine->log(ouzel::Log::Level::ERR) << e.what();
        return kCVReturnError;
    }

    [pool release];

    return kCVReturnSuccess;
}

namespace ouzel
{
    namespace graphics
    {
        namespace opengl
        {
            RenderDeviceMacOS::RenderDeviceMacOS(const std::function<void(const Event&)>& initCallback):
                RenderDevice(initCallback)
            {
            }

            RenderDeviceMacOS::~RenderDeviceMacOS()
            {
                running = false;
                CommandBuffer commandBuffer;
                commandBuffer.pushCommand(std::unique_ptr<Command>(new PresentCommand()));
                submitCommandBuffer(std::move(commandBuffer));

                if (displayLink)
                {
                    CVDisplayLinkStop(displayLink);
                    CVDisplayLinkRelease(displayLink);
                }

                if (openGLContext)
                {
                    [NSOpenGLContext clearCurrentContext];
                    [openGLContext release];
                }

                if (pixelFormat)
                    [pixelFormat release];
            }

            void RenderDeviceMacOS::init(Window* newWindow,
                                         const Size2U& newSize,
                                         uint32_t newSampleCount,
                                         Texture::Filter newTextureFilter,
                                         uint32_t newMaxAnisotropy,
                                         bool newSrgb,
                                         bool newVerticalSync,
                                         bool newDepth,
                                         bool newStencil,
                                         bool newDebugRenderer)
            {
                for (NSOpenGLPixelFormatAttribute openGLVersion : {NSOpenGLProfileVersion4_1Core, NSOpenGLProfileVersion3_2Core, NSOpenGLProfileVersionLegacy})
                {
                    // Create pixel format
                    std::vector<NSOpenGLPixelFormatAttribute> attributes =
                    {
                        NSOpenGLPFAAccelerated,
                        NSOpenGLPFANoRecovery,
                        NSOpenGLPFADoubleBuffer,
                        NSOpenGLPFAOpenGLProfile, openGLVersion,
                        NSOpenGLPFAColorSize, 24,
                        NSOpenGLPFAAlphaSize, 8,
                        NSOpenGLPFADepthSize, static_cast<NSOpenGLPixelFormatAttribute>(newDepth ? 24 : 0),
                        NSOpenGLPFAStencilSize, static_cast<NSOpenGLPixelFormatAttribute>(newStencil ? 8 : 0)
                    };

                    if (newSampleCount)
                    {
                        attributes.push_back(NSOpenGLPFAMultisample);
                        attributes.push_back(NSOpenGLPFASampleBuffers);
                        attributes.push_back(1);
                        attributes.push_back(NSOpenGLPFASamples);
                        attributes.push_back(newSampleCount);
                    }

                    attributes.push_back(0);

                    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes.data()];

                    if (pixelFormat)
                    {
                        switch (openGLVersion)
                        {
                            case NSOpenGLProfileVersionLegacy:
                                apiMajorVersion = 2;
                                apiMinorVersion = 0;
                                engine->log(Log::Level::INFO) << "OpenGL 2 pixel format created";
                                break;
                            case NSOpenGLProfileVersion3_2Core:
                                apiMajorVersion = 3;
                                apiMinorVersion = 2;
                                engine->log(Log::Level::INFO) << "OpenGL 3.2 pixel format created";
                                break;
                            case NSOpenGLProfileVersion4_1Core:
                                apiMajorVersion = 4;
                                apiMinorVersion = 1;
                                engine->log(Log::Level::INFO) << "OpenGL 4.1 pixel format created";
                                break;
                        }
                        break;
                    }
                }

                if (!pixelFormat)
                    throw std::runtime_error("Failed to crete OpenGL pixel format");

                // Create OpenGL context
                openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
                [openGLContext makeCurrentContext];

                NativeWindowMacOS* windowMacOS = static_cast<NativeWindowMacOS*>(newWindow->getNativeWindow());
                OpenGLView* openGLView = (OpenGLView*)windowMacOS->getNativeView();

                [openGLView setOpenGLContext:openGLContext];
                [openGLContext setView:openGLView];

                GLint swapInt = newVerticalSync ? 1 : 0;
                [openGLContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

                RenderDevice::init(newWindow,
                                      newSize,
                                      newSampleCount,
                                      newTextureFilter,
                                      newMaxAnisotropy,
                                      newSrgb,
                                      newVerticalSync,
                                      newDepth,
                                      newStencil,
                                      newDebugRenderer);

                eventHandler.windowHandler = std::bind(&RenderDeviceMacOS::handleWindow, this, std::placeholders::_1);
                engine->getEventDispatcher().addEventHandler(eventHandler);

                CGDirectDisplayID displayId = windowMacOS->getDisplayId();
                if (CVDisplayLinkCreateWithCGDisplay(displayId, &displayLink) != kCVReturnSuccess)
                    throw std::runtime_error("Failed to create display link");

                if (CVDisplayLinkSetOutputCallback(displayLink, ::renderCallback, this) != kCVReturnSuccess)
                    throw std::runtime_error("Failed to set output callback for the display link");

                running = true;

                if (CVDisplayLinkStart(displayLink) != kCVReturnSuccess)
                    throw std::runtime_error("Failed to start display link");
            }

            void RenderDeviceMacOS::resizeFrameBuffer()
            {
                [openGLContext update];
            }

            std::vector<Size2U> RenderDeviceMacOS::getSupportedResolutions() const
            {
                std::vector<Size2U> result;

                CFArrayRef displayModes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, nullptr);
                const CFIndex displayModeCount = CFArrayGetCount(displayModes);

                for (CFIndex i = 0; i < displayModeCount; ++i)
                {
                    const CGDisplayModeRef displayMode = (const CGDisplayModeRef)CFArrayGetValueAtIndex(displayModes, i);

                    result.push_back(Size2U(static_cast<uint32_t>(CGDisplayModeGetWidth(displayMode)),
                                            static_cast<uint32_t>(CGDisplayModeGetHeight(displayMode))));
                }

                CFRelease(displayModes);

                return result;
            }

            void RenderDeviceMacOS::present()
            {
                [openGLContext flushBuffer];
            }

            bool RenderDeviceMacOS::handleWindow(const WindowEvent& event)
            {
                if (event.type == ouzel::Event::Type::SCREEN_CHANGE)
                {
                    engine->executeOnMainThread([this, event]() {
                        if (displayLink)
                        {
                            CVDisplayLinkStop(displayLink);
                            CVDisplayLinkRelease(displayLink);
                            displayLink = nullptr;
                        }

                        const CGDirectDisplayID displayId = event.screenId;

                        if (CVDisplayLinkCreateWithCGDisplay(displayId, &displayLink) != kCVReturnSuccess)
                            throw std::runtime_error("Failed to create display link");

                        if (CVDisplayLinkSetOutputCallback(displayLink, ::renderCallback, this) != kCVReturnSuccess)
                            throw std::runtime_error("Failed to set output callback for the display link");

                        if (CVDisplayLinkStart(displayLink) != kCVReturnSuccess)
                            throw std::runtime_error("Failed to start display link");
                    });
                }

                return false;
            }

            void RenderDeviceMacOS::renderCallback()
            {
                if ([NSOpenGLContext currentContext] != openGLContext)
                    [openGLContext makeCurrentContext];

                if (running) process();
            }
        } // namespace opengl
    } // namespace graphics
} // namespace ouzel

#endif
