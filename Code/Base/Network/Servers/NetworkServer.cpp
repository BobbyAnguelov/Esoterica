#include "NetworkServer.h"

//-------------------------------------------------------------------------

namespace EE::Network
{
    Server::Server()
        : m_receivedMessages( 512 * Threading::TLockFreeQueue<Message*>::BLOCK_SIZE )
    {}

    void Server::Update( TFunction<void( Message const& )> const& handlerFunc )
    {
        EE_ASSERT( Threading::IsMainThread() );

        // Update connection status and receive network messages
        //-------------------------------------------------------------------------

        UpdateConnectionAndReceiveMessages();

        // Handle Incoming Messages
        //-------------------------------------------------------------------------

        Message* pMessage = nullptr;
        while ( m_receivedMessages.try_dequeue( pMessage ) )
        {
            EE_ASSERT( pMessage != nullptr );

            if ( pMessage->GetMessageID() == Message::HeartbeatID )
            {
                ReceiveHeartbeat( pMessage->GetClientID() );
            }
            else if( handlerFunc != nullptr )
            {
                handlerFunc( *pMessage );
            }

            EE::Delete( pMessage );
        }

        // Handle Heartbeats
        //-------------------------------------------------------------------------

        for ( int32_t i = (int32_t) m_connectedClients.size() - 1; i >= 0; i-- )
        {
            ClientInfo& client = m_connectedClients[i];

            if ( client.m_heartbeatTimer.IsRunning() )
            {
                client.m_heartbeatTimer.Update();
            }

            //-------------------------------------------------------------------------

            if ( client.m_heartbeatTimer.IsComplete() )
            {
                if ( !client.m_isConnected )
                {
                    DisconnectClient( client.m_ID );
                }
                else
                {
                    SendNetworkMessage( client.m_ID, Message( Message::HeartbeatID ) );
                    client.m_heartbeatTimer.Start( s_keepAliveIntervalSeconds );
                    client.m_isConnected = false;
                }
            }
        }

        // Send Outgoing Messages
        //-------------------------------------------------------------------------

        SendOutgoingMessages();
    }

    Server::ClientInfo* Server::GetConnectedClient( uint64_t clientID )
    {
        for ( auto& client : m_connectedClients )
        {
            if ( client.m_ID == clientID )
            {
                return &client;
            }
        }

        return nullptr;
    }

    void Server::ConnectClient( uint64_t clientID, char const* pAddress, void* pUserData )
    {
        EE_ASSERT( !HasConnectedClient( clientID ) );
        m_connectedClients.emplace_back( clientID, pAddress );
    }

    void Server::DisconnectClient( uint64_t clientID )
    {
        for ( int32_t i = 0; i < (int32_t) m_connectedClients.size(); i++ )
        {
            if ( m_connectedClients[i].m_ID == clientID )
            {
                m_connectedClients.erase( m_connectedClients.begin() + i );
                break;
            }
        }
    }

    void Server::ReceiveHeartbeat( uint64_t clientID )
    {
        for ( ClientInfo& connectedClient : m_connectedClients )
        {
            if ( connectedClient.m_ID == clientID )
            {
                connectedClient.m_isConnected = true;
                break;
            }
        }
    }
}