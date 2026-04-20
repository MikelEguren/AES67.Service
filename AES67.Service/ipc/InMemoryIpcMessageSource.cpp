#include "ipc/InMemoryIpcMessageSource.hpp"

namespace aes67::ipc
{
    InMemoryIpcMessageSource::InMemoryIpcMessageSource()
        : _messages(),
        _nextMessageIndex(0)
    {
        _messages.push_back("PREPARE|service-demo.wav");
        _messages.push_back("START|S-1");
        _messages.push_back("FINISH|S-1");
    }

    IpcReceiveResult InMemoryIpcMessageSource::ReceiveMessages()
    {
        IpcReceiveResult result;
        result.Success = true;

        if (_nextMessageIndex < _messages.size())
        {
            result.Messages.push_back(_messages[_nextMessageIndex]);
            ++_nextMessageIndex;
        }

        return result;
    }
}
