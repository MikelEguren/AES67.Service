#pragma once

#include <string>

#include "ipc/IIpcMessageSource.hpp"

namespace aes67::ipc
{
    class UnixDomainSocketIpcMessageSource : public IIpcMessageSource
    {
    public:
        explicit UnixDomainSocketIpcMessageSource(const std::string& socketPath);
        ~UnixDomainSocketIpcMessageSource() override;

        IpcReceiveResult ReceiveMessages() override;

    private:
        std::string _socketPath;
        bool _isInitialized;

#if defined(__linux__)
        int _serverSocket;
#endif

        bool EnsureInitialized(IpcReceiveResult& result);
        void Cleanup();
    };
}
