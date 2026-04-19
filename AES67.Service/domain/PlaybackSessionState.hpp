#pragma once

namespace aes67::domain
{
    enum class PlaybackSessionState
    {
        Created,
        Ready,
        Playing,
        Finished,
        Failed
    };
}