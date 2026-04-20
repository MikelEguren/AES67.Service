#include "ipc/InMemoryIpcMessageSource.hpp"

namespace aes67::ipc
{
    InMemoryIpcMessageSource::InMemoryIpcMessageSource()
    {}

    IpcReceiveResult InMemoryIpcMessageSource::ReceiveMessages()
    {
        IpcReceiveResult result;
        result.Success = true;

        result.Messages.push_back("PREPARE|service-demo.wav");

        return result;
    }
}
