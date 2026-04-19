#pragma once

#include "ChannelState.hpp"

namespace aes67::domain
{
    struct ChannelInfo
    {
        int ChannelNumber{ 0 };
        ChannelState State{ ChannelState::Free };
    };
}