#include "NetworkClient_WebSockets.h"
#include "Base/Memory/Memory.h"

#if EE_DEVELOPMENT_TOOLS
#include <ixwebsocket/IXWebSocket.h>
#include "ixwebsocket/IXWebSocketSendData.h"

//-------------------------------------------------------------------------

namespace EE::Network
{
    Client_WS::~Client_WS()
    {
        EE_ASSERT( m_pSocket == nullptr );
    }

    bool Client_WS::Start( char const* pAddress, uint16_t port )
    {
        auto receiveCallback = [this] ( const ix::WebSocketMessagePtr& msg )
        {
            Memory::InitializeThreadHeap();

            switch ( msg->type )
            {
                case ix::WebSocketMessageType::Message:
                {
                    auto pMessage = EE::New<Message>( msg->str.data(), msg->str.size() );
                    bool const wasEnqueued = m_receivedMessages.enqueue( pMessage );
                    EE_ASSERT( wasEnqueued );
                }
                break;

                case ix::WebSocketMessageType::Open:
                {
                    m_reconnectionAttemptsRemaining = s_numReconnectionAttempts;
                    m_status = Status::Connected;
                }
                break;

                case ix::WebSocketMessageType::Close:
                {
                    EE_LOG_ERROR( LogCategory::Network, "Websocket Client", "Remote server closed the connection: %s", msg->closeInfo.reason.c_str() );
                    m_status = Status::ConnectionFailed;
                }
                break;

                case ix::WebSocketMessageType::Error:
                {
                    EE_LOG_ERROR( LogCategory::Network, "Websocket Client", "Error occurred: %s", msg->errorInfo.reason.c_str() );
                }
                break;

                case ix::WebSocketMessageType::Fragment:
                {
                    EE_UNIMPLEMENTED_FUNCTION();
                }
                break;

                // Ping/Pong
                case ix::WebSocketMessageType::Ping:
                {
                    //EE_LOG_MESSAGE( LogCategory::Network, "Websocket Client", "Client Ping" );
                }
                break;

                case ix::WebSocketMessageType::Pong:
                {
                    //EE_LOG_MESSAGE( LogCategory::Network, "Websocket Client", "Client Pong" );
                }
                break;

                default:
                break;
            }
        };

        //-------------------------------------------------------------------------

        EE_ASSERT( m_pSocket == nullptr );

        m_address = pAddress;
        m_port = port;

        InlineString str( InlineString::CtorSprintf(), "ws://%s:%d/", m_address.c_str(), m_port );
        m_pSocket = EE::New<ix::WebSocket>();
        m_pSocket->setUrl( str.c_str() );
        m_pSocket->setPingInterval( 30 );
        m_pSocket->setOnMessageCallback( receiveCallback );
        m_pSocket->enableAutomaticReconnection();
        m_pSocket->setMaxWaitBetweenReconnectionRetries( s_reconnectionAttemptCooldownMilliseconds );

        // Create the connection and wait for an async response
        m_status = Status::Connecting;
        m_pSocket->start();

        return m_status != Status::ConnectionFailed && m_status != Status::Disconnected;
    }

    void Client_WS::Stop()
    {
        if ( m_pSocket != nullptr )
        {
            m_pSocket->stop( 1000, "Normal closure" );
            EE::Delete( m_pSocket );
        }
    }

    void Client_WS::SendOutgoingMessages()
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( IsConnected() );

        for ( Message const& msg : m_outgoingMessages )
        {
            ix::WebSocketSendInfo info = m_pSocket->sendBinary( ix::IXWebSocketSendData( msg.GetTransmissionData(), msg.GetTransmissionSize() ) );
            EE_ASSERT( info.success );
            EE_ASSERT( !info.compressionError );
        }

        m_outgoingMessages.clear();
    }
}
#endif