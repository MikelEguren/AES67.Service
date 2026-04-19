#pragma once

#include <vector>

#include "domain/ChannelInfo.hpp"

namespace aes67::engine
{
    class ChannelManager
    {
    public:
        explicit ChannelManager(int channelCount);

        const std::vector<aes67::domain::ChannelInfo>& GetChannels() const;

    private:
        std::vector<aes67::domain::ChannelInfo> _channels;
    };
}