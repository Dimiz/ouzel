// Copyright 2015-2019 Elviss Strazdins. All rights reserved.

#ifndef OUZEL_CORE_ENGINETVOS_HPP
#define OUZEL_CORE_ENGINETVOS_HPP

#if defined(__OBJC__)
typedef NSAutoreleasePool* NSAutoreleasePoolPtr;
#else
#  include <objc/NSObjCRuntime.h>
typedef id NSAutoreleasePoolPtr;
#endif

#include "core/Engine.hpp"

namespace ouzel
{
    class EngineTVOS final: public Engine
    {
    public:
        EngineTVOS(int initArgc, char* initArgv[]);
        ~EngineTVOS();

        void run();

        void openUrl(const std::string& url) final;

        void setScreenSaverEnabled(bool newScreenSaverEnabled) final;

        void executeAll();

    private:
        void runOnMainThread(const std::function<void()>& func) final;

        NSAutoreleasePoolPtr pool = nullptr;

        std::queue<std::function<void()>> executeQueue;
        std::mutex executeMutex;

        int argc = 0;
        char** argv = nullptr;
        id executeHanlder = nil;
    };
}

#endif // OUZEL_CORE_ENGINETVOS_HPP
