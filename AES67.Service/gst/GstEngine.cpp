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
#if defined(__linux__)
        if (_pipeline)
        {
            GstElement* pipeline = static_cast<GstElement*>(_pipeline);
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
            _pipeline = nullptr;
        }
#endif

        _initialized = false;
    }

    bool GstEngine::PlayFile(const std::string& path)
    {
#if defined(__linux__)
        if (!_initialized)
        {
            aes67::infra::Logger::Error("GStreamer not initialized.");
            return false;
        }

        std::string uri = "file://" + path;

        GstElement* pipeline = gst_element_factory_make("playbin", "player");
        if (!pipeline)
        {
            aes67::infra::Logger::Error("Failed to create playbin pipeline.");
            return false;
        }

        g_object_set(pipeline, "uri", uri.c_str(), NULL);

        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE)
        {
            aes67::infra::Logger::Error("Failed to start playback.");
            gst_object_unref(pipeline);
            return false;
        }

        aes67::infra::Logger::Info(("Playing file: " + path).c_str());

        _pipeline = pipeline;

        return true;
#else
        aes67::infra::Logger::Info("PlayFile skipped (non-Linux platform).");
        return true;
#endif
    }
}