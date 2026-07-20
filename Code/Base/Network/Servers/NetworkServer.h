#pragma once
#include "Base/_Module/API.h"
#include "Base/Network/NetworkMessage.h"
#include "Base/Types/Function.h"
#include "Base/Types/String.h"
#include "Base/Threading/Threading.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Network
{
    class EE_BASE_API Server
    {
        constexpr static float const s_keepAliveIntervalSeconds = 15.0f;

    public:

        struct ClientInfo
        {
            friend Server;

        public:

            ClientInfo() = default;

            ClientInfo( uint64_t ID, InlineString const& address )
                : m_ID( ID )
                , m_address( address )
            {
                EE_ASSERT( m_ID != 0 && !m_address.empty() );
            }

        public:

            uint64_t                                    m_ID;
            InlineString                                m_address;

        private:

            CountdownTimer<PlatformClock>               m_heartbeatTimer; // Time remaining to wait for a response
            bool                                        m_isConnected = true; // Updated when we receive a pong message
        };

    public:

        Server();
        virtual ~Server() = default;

        virtual bool Start( uint16_t port ) = 0;
        virtual void Stop() = 0;
        virtual bool IsRunning() const = 0;
        void Update( TFunction<void( Message const& )> const& handlerFunc );

        // Connection
        //-------------------------------------------------------------------------

        inline uint16_t const& GetPort() const { return m_port; }

        // Messaging
        //-------------------------------------------------------------------------

        // Queues a message to be sent to a specific client. Note this is a destructive operation!! This call will move the data
        inline void SendNetworkMessage( uint64_t clientID, Message&& msg ) { EE_ASSERT( clientID != 0 ); EE_ASSERT( Threading::IsMainThread() ); m_outgoingMessages.emplace_back( eastl::move( msg ) ).SetClientID( clientID ); }

        // Queues a message to be sent to a specific client
        inline void SendNetworkMessage( uint64_t clientID, Message const& msg ) { EE_ASSERT( clientID != 0 ); EE_ASSERT( Threading::IsMainThread() ); m_outgoingMessages.emplace_back( msg ).SetClientID( clientID ); }

        // Queues a message to be sent to all clients. Note this is a destructive operation!! This call will move the data
        inline void SendNetworkMessage( Message&& msg ) { EE_ASSERT( msg.GetClientID() != 0 ); EE_ASSERT( Threading::IsMainThread() ); m_outgoingMessages.emplace_back( eastl::move( msg ) ); }

        // Queues a message to be sent to all clients
        inline void SendNetworkMessage( Message const& msg ) { EE_ASSERT( msg.GetClientID() != 0 ); EE_ASSERT( Threading::IsMainThread() ); m_outgoingMessages.emplace_back( msg ); }

        // Client info
        //-------------------------------------------------------------------------

        inline int32_t GetNumConnectedClients() const { return (int32_t) m_connectedClients.size(); }

        bool HasConnectedClient( uint64_t clientID ) const { return GetConnectedClientInfo( clientID ) != nullptr; }

        inline TVector<ClientInfo> const& GetConnectedClients() const { return m_connectedClients; }

        inline ClientInfo const& GetConnectedClientInfo( int32_t clientIdx ) const
        {
            EE_ASSERT( clientIdx >= 0 && clientIdx < (int32_t) m_connectedClients.size() );
            return m_connectedClients[clientIdx];
        }

        ClientInfo const* GetConnectedClientInfo( uint64_t clientID ) const { return const_cast<Server*>( this )->GetConnectedClient( clientID ); }

        InlineString const& GetConnectedClientAddress( int32_t clientIdx ) const
        {
            EE_ASSERT( clientIdx >= 0 && clientIdx < (int32_t) m_connectedClients.size() );
            return m_connectedClients[clientIdx].m_address;
        }

        InlineString GetConnectedClientAddress( uint64_t clientID ) const
        {
            if ( auto pClientInfo = GetConnectedClientInfo( clientID ) )
            {
                return pClientInfo->m_address;
            }

            EE_UNREACHABLE_CODE();
            return InlineString();
        }

    protected:

        virtual void UpdateConnectionAndReceiveMessages() = 0;
        virtual void SendOutgoingMessages() = 0;

        ClientInfo* GetConnectedClient( uint64_t clientID );

        void ReceiveHeartbeat( uint64_t clientID );

        virtual void ConnectClient( uint64_t clientID, char const* pAddress, void* pUserData );
        virtual void DisconnectClient( uint64_t clientID );

    private:

        Server& operator=( Server const& ) = delete;

    protected:

        uint16_t                                    m_port = 0;
        TVector<ClientInfo>                         m_connectedClients;
        Threading::TLockFreeQueue<Message*>         m_receivedMessages;
        TVector<Message>                            m_outgoingMessages;
    };
}