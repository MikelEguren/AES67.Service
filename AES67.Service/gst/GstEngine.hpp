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
        const std::string& GetLastError() const;
        bool Stop();

    private:
        bool _initialized{ false };
        std::string _lastError;

#if defined(__linux__)
        void* _pipeline{ nullptr };
#endif
    };
}