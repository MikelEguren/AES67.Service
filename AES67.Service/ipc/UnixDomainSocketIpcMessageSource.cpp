#include "ipc/UnixDomainSocketIpcMessageSource.hpp"

#if defined(__linux__)
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sys/time.h>
#endif

namespace aes67::ipc
{
    UnixDomainSocketIpcMessageSource::UnixDomainSocketIpcMessageSource(const std::string& socketPath)
        : _socketPath(socketPath),
        _isInitialized(false)
#if defined(__linux__)
        , _serverSocket(-1)
#endif
    {}

    UnixDomainSocketIpcMessageSource::~UnixDomainSocketIpcMessageSource()
    {
        Cleanup();
    }

    bool UnixDomainSocketIpcMessageSource::EnsureInitialized(IpcReceiveResult& result)
    {
#if defined(__linux__)
        char buffer[1024];

        const int clientSocket = ::accept(_serverSocket, nullptr, nullptr);
        if (clientSocket < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                result.Success = true;
                return result;
            }

            result.Success = false;
            result.ErrorMessage = std::string("Failed to accept unix domain socket connection: ") + std::strerror(errno);
            return result;
        }

        const ssize_t bytesRead = ::read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead < 0)
        {
            result.Success = false;
            result.ErrorMessage = std::string("Failed to read unix domain socket message: ") + std::strerror(errno);
            ::close(clientSocket);
            return result;
        }

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            result.Messages.push_back(std::string(buffer));
        }

        ::close(clientSocket);
        result.Success = true;
#else
        result.Success = false;
        result.ErrorMessage = "Unix domain socket message source is only supported on Linux.";
        return false;
#endif
    }

    void UnixDomainSocketIpcMessageSource::Cleanup()
    {
#if defined(__linux__)
        if (_serverSocket >= 0)
        {
            ::close(_serverSocket);
            _serverSocket = -1;
        }

        if (!_socketPath.empty())
        {
            ::unlink(_socketPath.c_str());
        }
#endif

        _isInitialized = false;
    }

    IpcReceiveResult UnixDomainSocketIpcMessageSource::ReceiveMessages()
    {
        IpcReceiveResult result;

        if (!EnsureInitialized(result))
        {
            return result;
        }

#if defined(__linux__)
        const int clientSocket = ::accept(_serverSocket, nullptr, nullptr);
        if (clientSocket < 0)
        {
            result.Success = false;
            result.ErrorMessage = std::string("Failed to accept unix domain socket connection: ") + std::strerror(errno);
            return result;
        }

        char buffer[1024];
        const int clientSocket = ::accept(_serverSocket, nullptr, nullptr);
        if (clientSocket < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                result.Success = true;
                return result;
            }

            result.Success = false;
            result.ErrorMessage = std::string("Failed to accept unix domain socket connection: ") + std::strerror(errno);
            return result;
        }

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            result.Messages.push_back(std::string(buffer));
        }

        ::close(clientSocket);
        result.Success = true;
#else
        result.Success = false;
        result.ErrorMessage = "Unix domain socket message source is only supported on Linux.";
#endif

        return result;
    }
}
