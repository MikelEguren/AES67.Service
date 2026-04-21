#include "GstEngine.hpp"

#include "infra/Logger.hpp"

#if defined(__linux__)
#include <gst/gst.h>
#endif

namespace aes67::gst
{
    GstEngine::GstEngine()
    {}

    GstEngine::~GstEngine()
    {
        Shutdown();
    }

    bool GstEngine::Initialize()
    {
        if (_initialized)
        {
            return true;
        }

#if defined(__linux__)
        int argc = 0;
        char** argv = nullptr;

        gst_init(&argc, &argv);

        aes67::infra::Logger::Info("GStreamer initialized.");
#else
        aes67::infra::Logger::Info("GStreamer integration is disabled on this platform.");
#endif

        _initialized = true;
        return true;
    }

    void GstEngine::Shutdown()
    {
        if (!_initialized)
        {
            return;
        }

#if defined(__linux__)
        aes67::infra::Logger::Info("GStreamer shutdown.");
#else
        aes67::infra::Logger::Info("GStreamer shutdown skipped on this platform.");
#endif

        _initialized = false;
    }
}