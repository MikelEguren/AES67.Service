#pragma once

namespace aes67::ipc
{
    enum class IpcCommandType
    {
        Unknown = 0,
        PreparePlayback = 1,
        StartPlayback = 2,
        FinishPlayback = 3
    };
}