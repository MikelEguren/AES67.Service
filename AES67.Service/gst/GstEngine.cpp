#include "GstEngine.hpp"

#include "infra/Logger.hpp"

#include <fstream>
#include <string>

#if defined(__linux__)
#include <gst/gst.h>
#endif

namespace aes67::gst
{
    namespace
    {
        bool FileExists(const std::string& path)
        {
            std::ifstream file(path);
            return file.good();
        }
    }

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
        for (auto& entry : _pipelines)
        {
            GstElement* pipeline = static_cast<GstElement*>(entry.second);
            if (pipeline)
            {
                gst_element_set_state(pipeline, GST_STATE_NULL);
                gst_object_unref(pipeline);
            }
        }

        _pipelines.clear();
        aes67::infra::Logger::Info("GStreamer shutdown.");
#else
        aes67::infra::Logger::Info("GStreamer shutdown skipped on this platform.");
#endif

        _initialized = false;
    }

    bool GstEngine::PlayFile(const std::string& sessionId, const std::string& path)
    {
        _lastError.clear();

        if (sessionId.empty())
        {
            _lastError = "Session id is empty.";
            aes67::infra::Logger::Error(_lastError.c_str());
            return false;
        }

        if (path.empty())
        {
            _lastError = "Playback path is empty.";
            aes67::infra::Logger::Error(_lastError.c_str());
            return false;
        }

        if (!FileExists(path))
        {
            _lastError = "Playback file does not exist: " + path;
            aes67::infra::Logger::Error(_lastError.c_str());
            return false;
        }

#if defined(__linux__)
        if (!_initialized)
        {
            _lastError = "GStreamer not initialized.";
            aes67::infra::Logger::Error(_lastError.c_str());
            return false;
        }

        auto existing = _pipelines.find(sessionId);
        if (existing != _pipelines.end())
        {
            GstElement* existingPipeline = static_cast<GstElement*>(existing->second);
            if (existingPipeline)
            {
                gst_element_set_state(existingPipeline, GST_STATE_NULL);
                gst_object_unref(existingPipeline);
            }

            _pipelines.erase(existing);
        }

        std::string uri = "file://" + path;
        aes67::infra::Logger::Info(("Playback URI: " + uri).c_str());

        std::string pipelineName = "player_" + sessionId;

        GstElement* pipeline = gst_element_factory_make("playbin", pipelineName.c_str());
        if (!pipeline)
        {
            _lastError = "Failed to create playbin pipeline.";
            aes67::infra::Logger::Error(_lastError.c_str());
            return false;
        }  return false;
        }

        g_object_set(pipeline, "uri", uri.c_str(), NULL);

        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE)
        {
            GstBus* bus = gst_element_get_bus(pipeline);
            if (bus)
            {
                GstMessage* message = gst_bus_timed_pop_filtered(
                    bus,
                    2 * GST_SECOND,
                    GST_MESSAGE_ERROR);

                if (message)
                {
                    GError* error = nullptr;
                    gchar* debugInfo = nullptr;
                    gst_message_parse_error(message, &error, &debugInfo);

                    if (error)
                    {
                        _lastError = std::string("Failed to start playback: ") + error->message;
                        g_error_free(error);
                    }
                    else
                    {
                        _lastError = "Failed to start playback.";
                    }

                    if (debugInfo)
                    {
                        aes67::infra::Logger::Error(debugInfo);
                        g_free(debugInfo);
                    }

                    gst_message_unref(message);
                }
                else
                {
                    _lastError = "Failed to start playback and no detailed GStreamer error was received.";
                }

                gst_object_unref(bus);
            }
            else
            {
                _lastError = "Failed to start playback and bus was not available.";
            }

            aes67::infra::Logger::Error(_lastError.c_str());
            gst_object_unref(pipeline);
            return false;
        }

        _pipelines[sessionId] = pipeline;

        aes67::infra::Logger::Info(("Playing file for session " + sessionId + ": " + path).c_str());
        return true;
#else
        aes67::infra::Logger::Info("PlayFile skipped (non-Linux platform).");
        return true;
#endif
    }

    bool GstEngine::Stop(const std::string& sessionId)
    {
#if defined(__linux__)
        auto existing = _pipelines.find(sessionId);
        if (existing == _pipelines.end())
        {
            aes67::infra::Logger::Info(("No active pipeline found for session " + sessionId + ".").c_str());
            return true;
        }

        GstElement* pipeline = static_cast<GstElement*>(existing->second);
        if (pipeline)
        {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
        }

        _pipelines.erase(existing);

        aes67::infra::Logger::Info(("Playback stopped for session " + sessionId + ".").c_str());
        return true;
#else
        aes67::infra::Logger::Info("Stop skipped (non-Linux platform).");
        return true;
#endif
    }

    const std::string& GstEngine::GetLastError() const
    {
        return _lastError;
    }
}