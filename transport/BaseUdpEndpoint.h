#pragma once

#include "concurrency/MpmcHashmap.h"
#include "jobmanager/JobQueue.h"
#include "memory/PacketPoolAllocator.h"
#include "transport/Endpoint.h"
#include "transport/RtcSocket.h"
#include "transport/RtcePoll.h"

namespace transport
{
class BaseUdpEndpoint : public Endpoint, RtcePoll::IEventListener
{
public:
    BaseUdpEndpoint(const char* name,
        jobmanager::JobManager& jobManager,
        size_t maxSessionCount,
        memory::PacketPoolAllocator& allocator,
        const SocketAddress& localPort,
        RtcePoll& epoll,
        bool isShared);

    virtual ~BaseUdpEndpoint();

    void sendTo(const transport::SocketAddress& target,
        memory::Packet* packet,
        memory::PacketPoolAllocator& allocator) override;

    void registerDefaultListener(IEvents* defaultListener) override;

    void start() override;
    bool openPort(uint16_t port);
    void closePort() override;

    SocketAddress getLocalPort() const override { return _socket.getBoundPort(); }

    bool configureBufferSizes(size_t sendBufferSize, size_t receiveBufferSize) override;

    bool isShared() const override { return _isShared; }

    const char* getName() const override { return _name.c_str(); }

    bool isGood() const { return _socket.isGood(); }

    Endpoint::State getState() const override { return _state; }

    ice::TransportType getTransportType() const override { return ice::TransportType::UDP; }

    EndpointMetrics getMetrics() const final { return EndpointMetrics(_receiveJobs.getCount(), _sendJobs.getCount()); }

public: // internal job interface
    // called on receiveJobs threads
    virtual void internalReceive(int fd, uint32_t batchSize);
    virtual void dispatchReceivedPacket(const SocketAddress& srcAddress, memory::Packet* packet) = 0;
    // called on sendJobs threads
    virtual void internalSend();

    virtual void internalClosePort(int countDown);

protected:
    std::atomic<Endpoint::State> _state;
    logger::LoggableId _name;
    SocketAddress _localPort;
    RtcSocket _socket;

    void onSocketPollStarted(int fd) override;
    void onSocketPollStopped(int fd) override;
    void onSocketReadable(int fd) override;
    void onSocketShutdown(int fd) override {}
    void onSocketWriteable(int fd) override {}

    struct OutboundPacket
    {
        transport::SocketAddress target;
        memory::Packet* packet;
        memory::PacketPoolAllocator* allocator;
    };

    jobmanager::JobQueue _receiveJobs;
    jobmanager::JobQueue _sendJobs;
    memory::PacketPoolAllocator& _allocator;
    concurrency::MpmcQueue<OutboundPacket> _sendQueue;

    RtcePoll& _epoll;
    const bool _isShared;
    std::atomic_flag _pendingRead = ATOMIC_FLAG_INIT;
    std::atomic_flag _pendingSend = ATOMIC_FLAG_INIT;
    std::atomic_flag _isFull = ATOMIC_FLAG_INIT;

    std::atomic<IEvents*> _defaultListener;
};
} // namespace transport
