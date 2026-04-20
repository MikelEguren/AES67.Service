#pragma once

#include <string>
#include <vector>

#include "ipc/IIpcMessageSource.hpp"

namespace aes67::ipc
{
    class InMemoryIpcMessageSource : public IIpcMessageSource
    {
    public:
        InMemoryIpcMessageSource();

        IpcReceiveResult ReceiveMessages() override;

    private:
        std::vector<std::string> _messages;
        std::size_t _nextMessageIndex;
    };
}