#pragma once
#include "Base/_Module/API.h"
#include "Base/Network/NetworkMessage.h"
#include "Base/Types/Function.h"
#include "Base/Types/String.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

namespace EE::Network
{
    class EE_BASE_API Client
    {
    public:

        enum class Status
        {
            Disconnected,
            Connecting,
            Connected,
            Reconnecting,
            ConnectionFailed,
        };

        constexpr static int32_t const s_numReconnectionAttempts = 5;
        constexpr static int32_t const s_reconnectionAttemptCooldownMilliseconds = 1000;

    public:

        Client();
        virtual ~Client() = default;

        virtual bool Start( char const* pAddress, uint16_t port ) = 0;
        virtual void Stop() = 0;
        void Update( TFunction<void( Message const& )> const& handlerFunc );

        // Connection
        //-------------------------------------------------------------------------

        inline TInlineString<64> const& GetAddress() const { return m_address; }
        inline uint16_t const& GetPort() const { return m_port; }

        inline bool IsConnected() const { return m_status == Status::Connected; }
        inline bool IsConnecting() const { return m_status == Status::Connecting || m_status == Status::Reconnecting; };
        inline bool IsReconnecting() const { return m_status == Status::Reconnecting; };
        inline bool HasConnectionFailed() const { return m_status == Status::ConnectionFailed; }
        inline bool IsDisconnected() const { return m_status == Status::Disconnected; }

        // Messaging
        //-------------------------------------------------------------------------

        // Queues a message to be sent. Note this is a destructive operation!! This call will move the data
        inline void SendNetworkMessage( Message&& msg ) 
        {
            EE_ASSERT( IsConnected() );
            EE_ASSERT( Threading::IsMainThread() );
            m_outgoingMessages.emplace_back( eastl::move( msg ) );
        }

        // Queues a message to be sent
        inline void SendNetworkMessage( Message const& msg )
        {
            EE_ASSERT( IsConnected() );
            EE_ASSERT( Threading::IsMainThread() );
            m_outgoingMessages.emplace_back( msg );
        }

    private:

        Client& operator=( Client const& ) = delete;

        virtual void UpdateConnectionAndReceiveMessages() = 0;
        virtual void SendOutgoingMessages() = 0;

    protected:

        TInlineString<64>                           m_address;
        uint16_t                                    m_port = 0;
        uint32_t                                    m_reconnectionAttemptsRemaining = s_numReconnectionAttempts;
        Status                                      m_status = Status::Disconnected;
        Threading::TLockFreeQueue<Message*>         m_receivedMessages;
        TVector<Message>                            m_outgoingMessages;
    };
}