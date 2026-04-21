#include "GstEngine.hpp"

#include "infra/Logger.hpp"
#include <string>

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
                _lastError.clear();

            #if defined(__linux__)
                            if (!_initialized)
                            {
                                _lastError = "GStreamer not initialized.";
                                aes67::infra::Logger::Error(_lastError.c_str());
                                return false;
                            }

                            if (_pipeline)
                            {
                                GstElement* existingPipeline = static_cast<GstElement*>(_pipeline);
                                gst_element_set_state(existingPipeline, GST_STATE_NULL);
                                gst_object_unref(existingPipeline);
                                _pipeline = nullptr;
                            }

                            std::string uri = "file://" + path;

                            GstElement* pipeline = gst_element_factory_make("playbin", "player");
                            if (!pipeline)
                            {
                                _lastError = "Failed to create playbin pipeline.";
                                aes67::infra::Logger::Error(_lastError.c_str());
                                return false;
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
                                        1000000000,
                                        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_STATE_CHANGED));

                                    if (message && GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
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
                                        _lastError = "Failed to start playback.";
                                        if (message)
                                        {
                                            gst_message_unref(message);
                                        }
                                    }

                                    gst_object_unref(bus);
                                }
                                else
                                {
                                    _lastError = "Failed to start playback.";
                                }

                                aes67::infra::Logger::Error(_lastError.c_str());
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
    const std::string& GstEngine::GetLastError() const
    {
        return _lastError;
    }
    bool GstEngine::Stop()
    {
    #if defined(__linux__)
            if (!_pipeline)
            {
                aes67::infra::Logger::Info("No active pipeline to stop.");
                return true;
            }

            GstElement* pipeline = static_cast<GstElement*>(_pipeline);

            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);

            _pipeline = nullptr;

            aes67::infra::Logger::Info("Playback stopped.");

            return true;
    #else
            aes67::infra::Logger::Info("Stop skipped (non-Linux platform).");
            return true;
    #endif
    }
}