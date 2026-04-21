#pragma once

#include <string>

namespace aes67::gst
{
    class GstEngine
    {
    public:
        GstEngine();
        ~GstEngine();

        bool Initialize();
        void Shutdown();

        bool PlayFile(const std::string& path);

    private:
        bool _initialized{ false };

#if defined(__linux__)
        void* _pipeline{ nullptr }; // GstElement*
#endif
    };
}