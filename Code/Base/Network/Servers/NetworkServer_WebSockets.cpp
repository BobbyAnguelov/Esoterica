#include "NetworkServer_WebSockets.h"
#include "Base/Memory/Memory.h"
#include "Base/Encoding/Hash.h"

#if EE_DEVELOPMENT_TOOLS
#include <ixwebsocket/IXWebSocketServer.h>

//-------------------------------------------------------------------------

namespace EE::Network
{
    Server_WS::~Server_WS()
    {
        EE_ASSERT( m_pServer == nullptr );
    }

    bool Server_WS::IsRunning() const
    {
        return ( m_pServer != nullptr );
    }

    bool Server_WS::Start( uint16_t port )
    {
        auto receiveCallback = [this] ( std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg )
        {
            Memory::InitializeThreadHeap();

            std::string const& ID = connectionState->getId();
            uint64_t clientID = Hash::GetHash64( ID.c_str(), ID.length() );

            switch ( msg->type )
            {
                case ix::WebSocketMessageType::Message:
                {
                    auto pMessage = EE::New<Message>( msg->str.data(), msg->str.size() );
                    pMessage->SetClientID( clientID );
                    bool const wasEnqueued = m_receivedMessages.enqueue( pMessage );
                    EE_ASSERT( wasEnqueued );

                    if ( !HasConnectedClient( clientID ) )
                    {
                        ConnectClient( clientID, connectionState->getRemoteIp().c_str(), &webSocket );
                    }
                }
                break;

                case ix::WebSocketMessageType::Error:
                {
                    EE_LOG_MESSAGE( LogCategory::Network, "Websocket Server", "Error occurred: %s", msg->errorInfo.reason.c_str() );
                }
                break;

                // Add connected client
                case ix::WebSocketMessageType::Open:
                {
                    ConnectClient( clientID, connectionState->getRemoteIp().c_str(), &webSocket );
                }
                break;

                // Remove connected client
                case ix::WebSocketMessageType::Close:
                {
                    EE_LOG_MESSAGE( LogCategory::Network, "Websocket Server", "Client Closed Connection: %s", msg->closeInfo.reason.c_str() );
                    DisconnectClient( clientID );
                }
                break;

                // Fragment
                case ix::WebSocketMessageType::Fragment:
                {
                    EE_UNIMPLEMENTED_FUNCTION();
                }
                break;

                // Ping/Pong
                case ix::WebSocketMessageType::Ping:
                {
                    //EE_LOG_MESSAGE( LogCategory::Network, "Websocket Server", "Server Ping" );
                }
                break;

                case ix::WebSocketMessageType::Pong:
                {
                    //EE_LOG_MESSAGE( LogCategory::Network, "Websocket Server", "Server Pong" );
                }
                break;

                default:
                break;
            };
        };

        //-------------------------------------------------------------------------

        EE_ASSERT( m_pServer == nullptr );

        m_port = port;
        m_pServer = EE::New<ix::WebSocketServer>( m_port, "127.0.0.1", 5, 128, 10, AF_INET, -1, -1 );
        m_pServer->setOnClientMessageCallback( receiveCallback );

        std::pair<bool, std::string> const res = m_pServer->listen();
        if ( !res.first )
        {
            EE_LOG_ERROR( LogCategory::Network, "Websocket Server", res.second.c_str() );
            EE::Delete( m_pServer );
            return false;
        }

        m_pServer->start();

        return true;
    }

    void Server_WS::Stop()
    {
        if ( m_pServer != nullptr )
        {
            m_pServer->stop();
            EE::Delete( m_pServer );
        }
    }

    void Server_WS::SendOutgoingMessages()
    {
        EE_ASSERT( m_pServer != nullptr && IsRunning() );
        EE_ASSERT( Threading::IsMainThread() );

        for ( auto& msg : m_outgoingMessages )
        {
            EE_ASSERT( msg.GetClientID() != 0 );
            ix::WebSocket* pSocket = m_clientSocketMap[msg.GetClientID()];
            if ( pSocket != nullptr ) // Clients could have disconnected by the time we need to send so the connection can have been destroyed
            {
                pSocket->sendBinary( ix::IXWebSocketSendData( msg.GetTransmissionData(), msg.GetTransmissionSize() ) );
            }
        }

        m_outgoingMessages.clear();
    }

    void Server_WS::ConnectClient( uint64_t clientID, char const* pAddress, void* pUserData )
    {
        Server::ConnectClient( clientID, pAddress, pUserData );

        EE_ASSERT( m_clientSocketMap.find( clientID ) == m_clientSocketMap.end() );
        m_clientSocketMap.insert_or_assign( clientID, (ix::WebSocket*) pUserData );
    }

    void Server_WS::DisconnectClient( uint64_t clientID )
    {
        Server::DisconnectClient( clientID );

        auto iter = m_clientSocketMap.find( clientID );
        EE_ASSERT( iter != m_clientSocketMap.end() );
        m_clientSocketMap.erase( iter );
    }
}
#endif