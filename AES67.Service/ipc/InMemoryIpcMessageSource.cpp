#include "ipc/InMemoryIpcMessageSource.hpp"

namespace aes67::ipc
{
    InMemoryIpcMessageSource::InMemoryIpcMessageSource()
    {}

    std::vector<std::string> InMemoryIpcMessageSource::ReceiveMessages()
    {
        std::vector<std::string> messages;
        messages.push_back("PREPARE|service-demo.wav");
        return messages;
    }
}
