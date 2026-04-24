#include "GstEngine.hpp"

#include "infra/Logger.hpp"

#include <fstream>
#include <string>

#if defined(__linux__)
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
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

#if defined(__linux__)
        GstFlowReturn OnNewSample(GstAppSink* sink, gpointer user_data)
        {
            static bool loggedCaps = false;

            GstSample* sample = gst_app_sink_pull_sample(sink);
            if (!sample)
            {
                return GST_FLOW_ERROR;
            }

            GstCaps* caps = gst_sample_get_caps(sample);

            if (caps && !loggedCaps)
            {
                GstStructure* structure = gst_caps_get_structure(caps, 0);

                const gchar* format = gst_structure_get_string(structure, "format");

                int rate = 0;
                int channels = 0;

                gst_structure_get_int(structure, "rate", &rate);
                gst_structure_get_int(structure, "channels", &channels);

                std::string msg = "Audio caps: format=" +
                    std::string(format ? format : "unknown") +
                    " rate=" + std::to_string(rate) +
                    " channels=" + std::to_string(channels);

                aes67::infra::Logger::Info(msg.c_str());

                loggedCaps = true;
            }

            GstBuffer* buffer = gst_sample_get_buffer(sample);
            if (buffer)
            {
                GstMapInfo map;
                if (gst_buffer_map(buffer, &map, GST_MAP_READ))
                {
                    // Aquí tienes audio real
                    // map.data → puntero a muestras
                    // map.size → tamaño en bytes

                    gst_buffer_unmap(buffer, &map);
                }
            }

            gst_sample_unref(sample);
            return GST_FLOW_OK;
        }
#endif
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
        }

        g_object_set(pipeline, "uri", uri.c_str(), NULL);

        GstElement* audioConvert = gst_element_factory_make("audioconvert", nullptr);
        GstElement* audioResample = gst_element_factory_make("audioresample", nullptr);
        GstElement* tee = gst_element_factory_make("tee", nullptr);

        GstElement* localQueue = gst_element_factory_make("queue", nullptr);
        GstElement* localSink = gst_element_factory_make("autoaudiosink", nullptr);

        GstElement* aes67Queue = gst_element_factory_make("queue", nullptr);
        GstElement* aes67Sink = gst_element_factory_make("appsink", nullptr);

        if (aes67Queue)
        {
            g_object_set(
                aes67Queue,
                "max-size-buffers", 2,
                "max-size-time", 0,
                "max-size-bytes", 0,
                "leaky", 2,
                NULL);
        }

        if (!audioConvert || !audioResample || !tee || !localQueue || !localSink || !aes67Queue || !aes67Sink)
        {
            _lastError = "Failed to create tee audio output elements.";
            aes67::infra::Logger::Error(_lastError.c_str());

            if (audioConvert) gst_object_unref(audioConvert);
            if (audioResample) gst_object_unref(audioResample);
            if (tee) gst_object_unref(tee);
            if (localQueue) gst_object_unref(localQueue);
            if (localSink) gst_object_unref(localSink);
            if (aes67Queue) gst_object_unref(aes67Queue);
            if (aes67Sink) gst_object_unref(aes67Sink);

            gst_object_unref(pipeline);
            return false;
        }

        g_object_set(
            aes67Sink,
            "emit-signals", TRUE,
            "sync", FALSE,
            "max-buffers", 2,
            "drop", TRUE,
            NULL);

        g_signal_connect(
            aes67Sink,
            "new-sample",
            G_CALLBACK(OnNewSample),
            nullptr);

        GstElement* audioBin = gst_bin_new(nullptr);
        if (!audioBin)
        {
            _lastError = "Failed to create audio bin.";
            aes67::infra::Logger::Error(_lastError.c_str());

            gst_object_unref(pipeline);
            return false;
        }

        gst_bin_add_many(
            GST_BIN(audioBin),
            audioConvert,
            audioResample,
            tee,
            localQueue,
            localSink,
            aes67Queue,
            aes67Sink,
            NULL);

        if (!gst_element_link_many(audioConvert, audioResample, tee, NULL))
        {
            _lastError = "Failed to link audio conversion chain.";
            aes67::infra::Logger::Error(_lastError.c_str());

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        if (!gst_element_link_many(localQueue, localSink, NULL))
        {
            _lastError = "Failed to link local audio branch.";
            aes67::infra::Logger::Error(_lastError.c_str());

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        if (!gst_element_link_many(aes67Queue, aes67Sink, NULL))
        {
            _lastError = "Failed to link AES67 appsink branch.";
            aes67::infra::Logger::Error(_lastError.c_str());

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        GstPad* teeLocalPad = gst_element_request_pad_simple(tee, "src_%u");
        GstPad* localQueueSinkPad = gst_element_get_static_pad(localQueue, "sink");

        if (!teeLocalPad || !localQueueSinkPad || gst_pad_link(teeLocalPad, localQueueSinkPad) != GST_PAD_LINK_OK)
        {
            _lastError = "Failed to link tee to local audio branch.";
            aes67::infra::Logger::Error(_lastError.c_str());

            if (teeLocalPad) gst_object_unref(teeLocalPad);
            if (localQueueSinkPad) gst_object_unref(localQueueSinkPad);

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        gst_object_unref(teeLocalPad);
        gst_object_unref(localQueueSinkPad);

        GstPad* teeAes67Pad = gst_element_request_pad_simple(tee, "src_%u");
        GstPad* aes67QueueSinkPad = gst_element_get_static_pad(aes67Queue, "sink");

        if (!teeAes67Pad || !aes67QueueSinkPad || gst_pad_link(teeAes67Pad, aes67QueueSinkPad) != GST_PAD_LINK_OK)
        {
            _lastError = "Failed to link tee to AES67 appsink branch.";
            aes67::infra::Logger::Error(_lastError.c_str());

            if (teeAes67Pad) gst_object_unref(teeAes67Pad);
            if (aes67QueueSinkPad) gst_object_unref(aes67QueueSinkPad);

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        gst_object_unref(teeAes67Pad);
        gst_object_unref(aes67QueueSinkPad);

        GstPad* sinkPad = gst_element_get_static_pad(audioConvert, "sink");
        if (!sinkPad)
        {
            _lastError = "Failed to get audioConvert sink pad.";
            aes67::infra::Logger::Error(_lastError.c_str());

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        GstPad* ghostPad = gst_ghost_pad_new("sink", sinkPad);
        gst_object_unref(sinkPad);

        if (!ghostPad)
        {
            _lastError = "Failed to create audio bin ghost pad.";
            aes67::infra::Logger::Error(_lastError.c_str());

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        if (!gst_element_add_pad(audioBin, ghostPad))
        {
            _lastError = "Failed to add ghost pad to audio bin.";
            aes67::infra::Logger::Error(_lastError.c_str());

            gst_object_unref(ghostPad);
            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        g_object_set(pipeline, "audio-sink", audioBin, NULL);

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