#include "GstEngine.hpp"

#include "infra/Logger.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(__linux__)
#include <arpa/inet.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <sys/socket.h>
#include <unistd.h>
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
        struct CapturedAudioBuffer
        {
            std::vector<unsigned char> Data;
            GstClockTime Pts{ GST_CLOCK_TIME_NONE };
            GstClockTime Duration{ GST_CLOCK_TIME_NONE };
        };

        struct Aes67PcmFrame
        {
            std::vector<unsigned char> Data;
        };

        struct RtpPacketInfo
        {
            std::uint16_t SequenceNumber{ 0 };
            std::uint32_t Timestamp{ 0 };
            std::uint32_t Ssrc{ 0 };
            std::size_t PayloadBytes{ 0 };
            std::vector<unsigned char> PacketBytes;
        };

        std::uint32_t BuildSsrcFromSessionId(const std::string& sessionId)
        {
            std::uint32_t hash = 2166136261u;

            for (char c : sessionId)
            {
                hash ^= static_cast<std::uint8_t>(c);
                hash *= 16777619u;
            }

            if (hash == 0)
            {
                hash = 0x12345678;
            }

            return hash;
        }
        int NormalizePacketTimeMs(int packetTimeMs)
        {
            if (packetTimeMs <= 0)
            {
                return 5;
            }

            if ((48000 * packetTimeMs) % 1000 != 0)
            {
                aes67::infra::Logger::Error("Invalid packetTimeMs. Falling back to 5 ms.");
                return 5;
            }

            return packetTimeMs;
        }
        void WriteSdpFile(
            const std::string& sessionId,
            const std::string& multicastIp,
            int port,
            std::uint32_t ssrc)
        {
            std::string sdpPath = "/tmp/aes67_" + sessionId + ".sdp";

            std::ofstream sdpFile(sdpPath);
            if (!sdpFile.is_open())
            {
                aes67::infra::Logger::Error("Failed to create SDP file.");
                return;
            }

            sdpFile
                << "v=0\n"
                << "o=- 0 0 IN IP4 127.0.0.1\n"
                << "s=AES67 Service " << sessionId << "\n"
                << "c=IN IP4 " << multicastIp << "/127\n"
                << "t=0 0\n"
                << "m=audio " << port << " RTP/AVP 96\n"
                << "a=rtpmap:96 L16/48000/1\n"
                << "a=recvonly\n"
                << "a=ssrc:" << ssrc << " cname:aes67-service-" << sessionId << "\n";

            sdpFile.close();

            aes67::infra::Logger::Info(("AES67 SDP file: " + sdpPath).c_str());
        }


        std::vector<unsigned char> BuildRtpPacket(
            std::uint16_t sequenceNumber,
            std::uint32_t timestamp,
            std::uint32_t ssrc,
            const std::vector<unsigned char>& payload)
        {
            std::vector<unsigned char> packet;
            packet.resize(12 + payload.size());

            packet[0] = 0x80;
            packet[1] = 96;

            packet[2] = static_cast<unsigned char>((sequenceNumber >> 8) & 0xFF);
            packet[3] = static_cast<unsigned char>(sequenceNumber & 0xFF);

            packet[4] = static_cast<unsigned char>((timestamp >> 24) & 0xFF);
            packet[5] = static_cast<unsigned char>((timestamp >> 16) & 0xFF);
            packet[6] = static_cast<unsigned char>((timestamp >> 8) & 0xFF);
            packet[7] = static_cast<unsigned char>(timestamp & 0xFF);

            packet[8] = static_cast<unsigned char>((ssrc >> 24) & 0xFF);
            packet[9] = static_cast<unsigned char>((ssrc >> 16) & 0xFF);
            packet[10] = static_cast<unsigned char>((ssrc >> 8) & 0xFF);
            packet[11] = static_cast<unsigned char>(ssrc & 0xFF);

            std::copy(payload.begin(), payload.end(), packet.begin() + 12);

            return packet;
        }

        struct SessionCaptureContext
        {
            std::string SessionId;
            int PacketTimeMs{ 0 };

            int SocketFd{ -1 };
            sockaddr_in DestAddr{};

            std::mutex Mutex;
            std::condition_variable Condition;
            std::deque<CapturedAudioBuffer> Queue;
            std::thread Worker;

            std::atomic<bool> StopRequested{ false };
            std::atomic<unsigned long long> ReceivedBuffers{ 0 };
            std::atomic<unsigned long long> DroppedBuffers{ 0 };
            std::atomic<unsigned long long> PreparedRtpPackets{ 0 };
            std::atomic<unsigned long long> PreparedPayloadBytes{ 0 };
            std::atomic<unsigned long long> SentRtpPackets{ 0 };
            std::atomic<unsigned long long> SendErrors{ 0 };

            std::vector<unsigned char> PendingPcmBytes;
            std::ofstream DebugRawFile;

            std::uint16_t RtpSequenceNumber{ 1 };
            std::uint32_t RtpTimestamp{ 0 };
            std::uint32_t RtpSsrc{ 0 };

            bool FirstPacketSendTimeInitialized{ false };
            std::chrono::steady_clock::time_point FirstSendTime;
            std::uint32_t FirstRtpTimestamp{ 0 };

            std::chrono::steady_clock::time_point FirstActualSendTime;
            bool FirstActualSendTimeInitialized{ false };
            long long DriftCorrectionNs{ 0 };

            bool CapsLogged{ false };
        };

        void CaptureWorker(SessionCaptureContext* context)
        {
            auto lastLogTime = std::chrono::steady_clock::now();
            std::size_t payloadBytesAccumulated = 0;
            std::size_t packetsAccumulated = 0;

            while (true)
            {
                CapturedAudioBuffer buffer;

                {
                    std::unique_lock<std::mutex> lock(context->Mutex);

                    context->Condition.wait(lock, [context]()
                        {
                            return context->StopRequested || !context->Queue.empty();
                        });

                    if (context->StopRequested && context->Queue.empty())
                    {
                        break;
                    }

                    buffer = std::move(context->Queue.front());
                    context->Queue.pop_front();
                }

                const int packetTimeMs = context->PacketTimeMs;

                const std::size_t SamplesPerFrame =
                    static_cast<std::size_t>(48000 * packetTimeMs / 1000);

                constexpr std::size_t BytesPerSample = 2;
                constexpr std::size_t Channels = 1;

                const std::size_t BytesPerFrame =
                    SamplesPerFrame * BytesPerSample * Channels;

                context->PendingPcmBytes.insert(
                    context->PendingPcmBytes.end(),
                    buffer.Data.begin(),
                    buffer.Data.end());

                while (context->PendingPcmBytes.size() >= BytesPerFrame)
                {
                    Aes67PcmFrame frame;
                    frame.Data.insert(
                        frame.Data.end(),
                        context->PendingPcmBytes.begin(),
                        context->PendingPcmBytes.begin() + BytesPerFrame);

                    context->PendingPcmBytes.erase(
                        context->PendingPcmBytes.begin(),
                        context->PendingPcmBytes.begin() + BytesPerFrame);

                    if (context->DebugRawFile.is_open())
                    {
                        context->DebugRawFile.write(
                            reinterpret_cast<const char*>(frame.Data.data()),
                            static_cast<std::streamsize>(frame.Data.size()));
                    }

                    RtpPacketInfo packet;
                    packet.SequenceNumber = context->RtpSequenceNumber++;
                    packet.Timestamp = context->RtpTimestamp;
                    packet.Ssrc = context->RtpSsrc;
                    packet.PayloadBytes = frame.Data.size();

                    packet.PacketBytes = BuildRtpPacket(
                        packet.SequenceNumber,
                        packet.Timestamp,
                        packet.Ssrc,
                        frame.Data);

                    if (packet.PacketBytes.size() != 12 + packet.PayloadBytes)
                    {
                        aes67::infra::Logger::Error("Invalid RTP packet size.");
                    }

                    context->RtpTimestamp += static_cast<std::uint32_t>(SamplesPerFrame);

                    ++context->PreparedRtpPackets;
                    context->PreparedPayloadBytes += packet.PayloadBytes;

                    if (context->SocketFd >= 0)
                    {
                        if (!context->FirstPacketSendTimeInitialized)
                        {
                            context->FirstPacketSendTimeInitialized = true;
                            context->FirstSendTime = std::chrono::steady_clock::now();
                            context->FirstRtpTimestamp = packet.Timestamp;
                        }

                        const std::uint32_t rtpTimestampOffset =
                            packet.Timestamp - context->FirstRtpTimestamp;

                        const long long scheduledNs =
                            static_cast<long long>(
                                (static_cast<std::uint64_t>(rtpTimestampOffset) * 1000000000ULL) / 48000ULL);

                        const auto targetSendTime =
                            context->FirstSendTime +
                            std::chrono::nanoseconds(scheduledNs - context->DriftCorrectionNs);

                        const auto nowForSend = std::chrono::steady_clock::now();

                        if (targetSendTime > nowForSend)
                        {
                            std::this_thread::sleep_until(targetSendTime);
                        }

                        const ssize_t sentBytes = sendto(
                            context->SocketFd,
                            packet.PacketBytes.data(),
                            packet.PacketBytes.size(),
                            0,
                            reinterpret_cast<sockaddr*>(&context->DestAddr),
                            sizeof(context->DestAddr));

                        if (sentBytes < 0)
                        {
                            ++context->SendErrors;
                        }
                        else
                        {
                            if (!context->FirstActualSendTimeInitialized)
                            {
                                context->FirstActualSendTimeInitialized = true;
                                context->FirstActualSendTime = std::chrono::steady_clock::now();
                            }

                            ++context->SentRtpPackets;
                        }
                    }

                    ++packetsAccumulated;
                    payloadBytesAccumulated += packet.PayloadBytes;
                }

                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastLogTime);

                if(elapsed.count() >= 1)
                {
                    long long scheduledMs = 0;
                    long long actualMs = 0;
                    long long driftMs = 0;

                    if (context->FirstPacketSendTimeInitialized && context->FirstActualSendTimeInitialized)
                    {
                        scheduledMs = static_cast<long long>(
                            (static_cast<std::uint64_t>(context->RtpTimestamp - context->FirstRtpTimestamp) * 1000ULL) / 48000ULL);

                        actualMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - context->FirstActualSendTime).count();

                        driftMs = actualMs - scheduledMs;

                        if (driftMs > 2 || driftMs < -2)
                        {
                            context->DriftCorrectionNs += (driftMs * 1000000LL) / 10;
                        }
                    }

                    std::string msg =
                        "AES67 RTP prepared [" + context->SessionId + "] " +
                        "packetTimeMs=" + std::to_string(packetTimeMs) +
                        " packets=" + std::to_string(packetsAccumulated) +
                        " payloadBytes=" + std::to_string(payloadBytesAccumulated) +
                        " timestampStep=" + std::to_string(SamplesPerFrame) +
                        " scheduledMs=" + std::to_string(scheduledMs) +
                        " actualMs=" + std::to_string(actualMs) +
                        " driftMs=" + std::to_string(driftMs) +
                        " correctionNs=" + std::to_string(context->DriftCorrectionNs);

                    aes67::infra::Logger::Info(msg.c_str());

                    payloadBytesAccumulated = 0;
                    packetsAccumulated = 0;
                    lastLogTime = now;
                }
            }
        }

        SessionCaptureContext* CreateCaptureContext(
            const std::string& sessionId,
            int packetTimeMs,
            const std::string& ip,
            int port,
            int multicastTtl)
        {
            SessionCaptureContext* context = new SessionCaptureContext();
            context->SessionId = sessionId;
            context->PacketTimeMs = NormalizePacketTimeMs(packetTimeMs);
            context->RtpSsrc = BuildSsrcFromSessionId(sessionId);
            WriteSdpFile(sessionId, ip, port, context->RtpSsrc);
            std::string rtpMessage =
                "AES67 RTP session " + sessionId +
                " SSRC=" + std::to_string(context->RtpSsrc) +
                " packetTimeMs=" + std::to_string(context->PacketTimeMs);

            aes67::infra::Logger::Info(rtpMessage.c_str());

            const std::size_t samplesPerFrame =
                static_cast<std::size_t>(48000 * context->PacketTimeMs / 1000);

            const std::size_t bytesPerFrame =
                samplesPerFrame * 2;

            std::string framingMessage =
                "AES67 framing " + sessionId +
                " samplesPerFrame=" + std::to_string(samplesPerFrame) +
                " bytesPerFrame=" + std::to_string(bytesPerFrame);

            aes67::infra::Logger::Info(framingMessage.c_str());

            std::string debugPath = "/tmp/aes67_capture_" + sessionId + ".raw";
            context->DebugRawFile.open(debugPath, std::ios::binary);

            if (context->DebugRawFile.is_open())
            {
                aes67::infra::Logger::Info(("AES67 debug RAW file: " + debugPath).c_str());
            }
            else
            {
                aes67::infra::Logger::Error("Failed to open AES67 debug RAW file.");
            }

            context->SocketFd = socket(AF_INET, SOCK_DGRAM, 0);
            if (context->SocketFd < 0)
            {
                aes67::infra::Logger::Error("Failed to create UDP socket.");
            }

            if (context->SocketFd >= 0)
            {
                unsigned char ttl = static_cast<unsigned char>(multicastTtl);

                if (setsockopt(
                    context->SocketFd,
                    IPPROTO_IP,
                    IP_MULTICAST_TTL,
                    &ttl,
                    sizeof(ttl)) < 0)
                {
                    aes67::infra::Logger::Error("Failed to set multicast TTL.");
                }
            }

            context->DestAddr.sin_family = AF_INET;
            context->DestAddr.sin_port = htons(static_cast<std::uint16_t>(port));

            if (inet_pton(AF_INET, ip.c_str(), &context->DestAddr.sin_addr) <= 0)
            {
                aes67::infra::Logger::Error("Invalid AES67 destination IP address.");
            }

            context->Worker = std::thread(CaptureWorker, context);

            return context;
        }

        void StopCaptureContext(SessionCaptureContext* context)
        {
            if (!context)
            {
                return;
            }

            context->StopRequested = true;
            context->Condition.notify_all();

            if (context->Worker.joinable())
            {
                context->Worker.join();
            }

            std::string message =
                "AES67 capture stopped for session " + context->SessionId +
                ". packetTimeMs=" + std::to_string(context->PacketTimeMs) +
                " received=" + std::to_string(context->ReceivedBuffers.load()) +
                " dropped=" + std::to_string(context->DroppedBuffers.load()) +
                " rtpPackets=" + std::to_string(context->PreparedRtpPackets.load()) +
                " rtpPayloadBytes=" + std::to_string(context->PreparedPayloadBytes.load()) +
                " sent=" + std::to_string(context->SentRtpPackets.load()) +
                " sendErrors=" + std::to_string(context->SendErrors.load());

            aes67::infra::Logger::Info(message.c_str());

            if (context->SocketFd >= 0)
            {
                close(context->SocketFd);
                context->SocketFd = -1;
            }

            if (context->DebugRawFile.is_open())
            {
                context->DebugRawFile.close();
            }

            delete context;
        }

        GstFlowReturn OnNewSample(GstAppSink* sink, gpointer userData)
        {
            SessionCaptureContext* context = static_cast<SessionCaptureContext*>(userData);
            if (!context)
            {
                return GST_FLOW_ERROR;
            }

            GstSample* sample = gst_app_sink_pull_sample(sink);
            if (!sample)
            {
                return GST_FLOW_ERROR;
            }

            GstCaps* caps = gst_sample_get_caps(sample);
            if (caps && !context->CapsLogged)
            {
                GstStructure* structure = gst_caps_get_structure(caps, 0);

                const gchar* format = gst_structure_get_string(structure, "format");

                int rate = 0;
                int channels = 0;

                gst_structure_get_int(structure, "rate", &rate);
                gst_structure_get_int(structure, "channels", &channels);

                std::string msg =
                    "AES67 capture caps for session " + context->SessionId +
                    ": format=" + std::string(format ? format : "unknown") +
                    " rate=" + std::to_string(rate) +
                    " channels=" + std::to_string(channels);

                aes67::infra::Logger::Info(msg.c_str());

                context->CapsLogged = true;
            }

            GstBuffer* gstBuffer = gst_sample_get_buffer(sample);
            if (gstBuffer)
            {
                GstMapInfo map;
                if (gst_buffer_map(gstBuffer, &map, GST_MAP_READ))
                {
                    CapturedAudioBuffer captured;
                    captured.Data.assign(map.data, map.data + map.size);
                    captured.Pts = GST_BUFFER_PTS(gstBuffer);
                    captured.Duration = GST_BUFFER_DURATION(gstBuffer);

                    {
                        std::lock_guard<std::mutex> lock(context->Mutex);

                        constexpr std::size_t MaxQueuedBuffers = 1000;

                        if (context->Queue.size() >= MaxQueuedBuffers)
                        {
                            context->Queue.pop_front();
                            ++context->DroppedBuffers;
                        }

                        context->Queue.push_back(std::move(captured));
                        ++context->ReceivedBuffers;
                    }

                    context->Condition.notify_one();

                    gst_buffer_unmap(gstBuffer, &map);
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

        for (auto& entry : _captures)
        {
            StopCaptureContext(static_cast<SessionCaptureContext*>(entry.second));
        }

        _captures.clear();

        aes67::infra::Logger::Info("GStreamer shutdown.");
#else
        aes67::infra::Logger::Info("GStreamer shutdown skipped on this platform.");
#endif

        _initialized = false;
    }

    bool GstEngine::PlayFile(
        const std::string& sessionId,
        const std::string& path,
        bool enableLocalMonitor,
        int packetTimeMs,
        const std::string& destIp,
        int destPort,
        int multicastTtl)
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

        auto existingCapture = _captures.find(sessionId);
        if (existingCapture != _captures.end())
        {
            StopCaptureContext(static_cast<SessionCaptureContext*>(existingCapture->second));
            _captures.erase(existingCapture);
        }

        std::string uri = "file://" + path;
        aes67::infra::Logger::Info(("Playback URI: " + uri).c_str());
        aes67::infra::Logger::Info(enableLocalMonitor
            ? "Local audio monitor enabled."
            : "Local audio monitor disabled.");

        std::string destinationMessage =
            "AES67 RTP destination: " + destIp + ":" + std::to_string(destPort) +
            " ttl=" + std::to_string(multicastTtl);

        aes67::infra::Logger::Info(destinationMessage.c_str());

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

        GstElement* localQueue = enableLocalMonitor ? gst_element_factory_make("queue", nullptr) : nullptr;
        GstElement* localSink = enableLocalMonitor ? gst_element_factory_make("autoaudiosink", nullptr) : nullptr;

        GstElement* aes67Queue = gst_element_factory_make("queue", nullptr);
        GstElement* aes67Convert = gst_element_factory_make("audioconvert", nullptr);
        GstElement* aes67Resample = gst_element_factory_make("audioresample", nullptr);
        GstElement* aes67CapsFilter = gst_element_factory_make("capsfilter", nullptr);
        GstElement* aes67Sink = gst_element_factory_make("appsink", nullptr);

        if (!audioConvert || !audioResample || !tee ||
            (enableLocalMonitor && (!localQueue || !localSink)) ||
            !aes67Queue || !aes67Convert || !aes67Resample || !aes67CapsFilter || !aes67Sink)
        {
            _lastError = "Failed to create tee audio output elements.";
            aes67::infra::Logger::Error(_lastError.c_str());

            if (audioConvert) gst_object_unref(audioConvert);
            if (audioResample) gst_object_unref(audioResample);
            if (tee) gst_object_unref(tee);
            if (localQueue) gst_object_unref(localQueue);
            if (localSink) gst_object_unref(localSink);
            if (aes67Queue) gst_object_unref(aes67Queue);
            if (aes67Convert) gst_object_unref(aes67Convert);
            if (aes67Resample) gst_object_unref(aes67Resample);
            if (aes67CapsFilter) gst_object_unref(aes67CapsFilter);
            if (aes67Sink) gst_object_unref(aes67Sink);

            gst_object_unref(pipeline);
            return false;
        }

        GstCaps* aes67Caps = gst_caps_new_simple(
            "audio/x-raw",
            "format", G_TYPE_STRING, "S16BE",
            "rate", G_TYPE_INT, 48000,
            "channels", G_TYPE_INT, 1,
            NULL);

        g_object_set(aes67CapsFilter, "caps", aes67Caps, NULL);
        gst_caps_unref(aes67Caps);

        SessionCaptureContext* captureContext =
            CreateCaptureContext(sessionId, packetTimeMs, destIp, destPort, multicastTtl);

        _captures[sessionId] = captureContext;

        g_object_set(
            aes67Sink,
            "emit-signals", TRUE,
            "sync", FALSE,
            "max-buffers", 1000,
            "drop", TRUE,
            NULL);

        g_signal_connect(
            aes67Sink,
            "new-sample",
            G_CALLBACK(OnNewSample),
            captureContext);

        GstElement* audioBin = gst_bin_new(nullptr);
        if (!audioBin)
        {
            _lastError = "Failed to create audio bin.";
            aes67::infra::Logger::Error(_lastError.c_str());

            StopCaptureContext(captureContext);
            _captures.erase(sessionId);

            gst_object_unref(pipeline);
            return false;
        }

        gst_bin_add_many(
            GST_BIN(audioBin),
            audioConvert,
            audioResample,
            tee,
            aes67Queue,
            aes67Convert,
            aes67Resample,
            aes67CapsFilter,
            aes67Sink,
            NULL);

        if (enableLocalMonitor)
        {
            gst_bin_add_many(
                GST_BIN(audioBin),
                localQueue,
                localSink,
                NULL);
        }

        if (!gst_element_link_many(audioConvert, audioResample, tee, NULL))
        {
            _lastError = "Failed to link audio conversion chain.";
            aes67::infra::Logger::Error(_lastError.c_str());

            StopCaptureContext(captureContext);
            _captures.erase(sessionId);

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        if (enableLocalMonitor)
        {
            if (!gst_element_link_many(localQueue, localSink, NULL))
            {
                _lastError = "Failed to link local audio branch.";
                aes67::infra::Logger::Error(_lastError.c_str());

                StopCaptureContext(captureContext);
                _captures.erase(sessionId);

                gst_object_unref(audioBin);
                gst_object_unref(pipeline);
                return false;
            }
        }

        if (!gst_element_link_many(aes67Queue, aes67Convert, aes67Resample, aes67CapsFilter, aes67Sink, NULL))
        {
            _lastError = "Failed to link AES67 appsink branch.";
            aes67::infra::Logger::Error(_lastError.c_str());

            StopCaptureContext(captureContext);
            _captures.erase(sessionId);

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        if (enableLocalMonitor)
        {
            GstPad* teeLocalPad = gst_element_request_pad_simple(tee, "src_%u");
            GstPad* localQueueSinkPad = gst_element_get_static_pad(localQueue, "sink");

            if (!teeLocalPad || !localQueueSinkPad || gst_pad_link(teeLocalPad, localQueueSinkPad) != GST_PAD_LINK_OK)
            {
                _lastError = "Failed to link tee to local audio branch.";
                aes67::infra::Logger::Error(_lastError.c_str());

                if (teeLocalPad) gst_object_unref(teeLocalPad);
                if (localQueueSinkPad) gst_object_unref(localQueueSinkPad);

                StopCaptureContext(captureContext);
                _captures.erase(sessionId);

                gst_object_unref(audioBin);
                gst_object_unref(pipeline);
                return false;
            }

            gst_object_unref(teeLocalPad);
            gst_object_unref(localQueueSinkPad);
        }

        GstPad* teeAes67Pad = gst_element_request_pad_simple(tee, "src_%u");
        GstPad* aes67QueueSinkPad = gst_element_get_static_pad(aes67Queue, "sink");

        if (!teeAes67Pad || !aes67QueueSinkPad || gst_pad_link(teeAes67Pad, aes67QueueSinkPad) != GST_PAD_LINK_OK)
        {
            _lastError = "Failed to link tee to AES67 appsink branch.";
            aes67::infra::Logger::Error(_lastError.c_str());

            if (teeAes67Pad) gst_object_unref(teeAes67Pad);
            if (aes67QueueSinkPad) gst_object_unref(aes67QueueSinkPad);

            StopCaptureContext(captureContext);
            _captures.erase(sessionId);

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

            StopCaptureContext(captureContext);
            _captures.erase(sessionId);

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

            StopCaptureContext(captureContext);
            _captures.erase(sessionId);

            gst_object_unref(audioBin);
            gst_object_unref(pipeline);
            return false;
        }

        if (!gst_element_add_pad(audioBin, ghostPad))
        {
            _lastError = "Failed to add ghost pad to audio bin.";
            aes67::infra::Logger::Error(_lastError.c_str());

            gst_object_unref(ghostPad);

            StopCaptureContext(captureContext);
            _captures.erase(sessionId);

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

            StopCaptureContext(captureContext);
            _captures.erase(sessionId);

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

        auto existingCapture = _captures.find(sessionId);
        if (existingCapture != _captures.end())
        {
            StopCaptureContext(static_cast<SessionCaptureContext*>(existingCapture->second));
            _captures.erase(existingCapture);
        }

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