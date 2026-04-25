#pragma once

#include <string>
#include <unordered_map>

namespace aes67::gst
{
    class GstEngine
    {
    public:
        GstEngine();
        ~GstEngine();

        bool Initialize();
        void Shutdown();

        bool PlayFile(
            const std::string& sessionId,
            const std::string& path,
            bool enableLocalMonitor,
            int packetTimeMs,
            const std::string& destIp,
            int destPort,
            int multicastTtl);
        bool Stop(const std::string& sessionId);
        const std::string& GetLastError() const;

    private:
        bool _initialized{ false };
        std::string _lastError;

#if defined(__linux__)
        std::unordered_map<std::string, void*> _pipelines;
        std::unordered_map<std::string, void*> _captures;
#endif
    };
}