#pragma once

namespace aes67::gst
{
    class GstEngine
    {
    public:
        GstEngine();
        ~GstEngine();

        bool Initialize();
        void Shutdown();

    private:
        bool _initialized{ false };
    };
}