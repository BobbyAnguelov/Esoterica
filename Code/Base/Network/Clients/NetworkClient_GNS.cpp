#include "NetworkClient_GNS.h"
#include "Base/Threading/Threading.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

//-------------------------------------------------------------------------

namespace EE::Network
{
    Client_GNS::~Client_GNS()
    {
        EE_ASSERT( m_connectionHandle == k_HSteamNetConnection_Invalid );
    }

    bool Client_GNS::Start( char const* pAddress, uint16_t port )
    {
        m_address = pAddress;
        m_port = port;
        return TryConnect();
    }

    bool Client_GNS::TryConnect()
    {
        InlineString str( InlineString::CtorSprintf(), "%s:%d", m_address.c_str(), m_port);
        SteamNetworkingIPAddr serverAddr;
        if ( !serverAddr.ParseString( str.c_str() ) )
        {
            EE_LOG_ERROR( LogCategory::Network, "Client Connection", "Invalid client IP address provided: %s", m_address.c_str() );
            return false;
        }

        if ( serverAddr.m_port == 0 )
        {
            EE_LOG_ERROR( LogCategory::Network, "Client Connection", "No port for client address provided: %s", m_address.c_str() );
            return false;
        }

        //-------------------------------------------------------------------------

        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*) ConnectionChangedCallback );
        m_connectionHandle = pInterface->ConnectByIPAddress( serverAddr, 1, &opt );

        if ( m_connectionHandle == k_HSteamNetConnection_Invalid )
        {
            EE_LOG_ERROR( LogCategory::Network, "Client Connection", "Failed to create connection" );
            return false;
        }

        bool const result = pInterface->SetConnectionUserData( m_connectionHandle, intptr_t( this ) );
        EE_ASSERT( result );
        m_status = Status::Connecting;
        return true;
    }

    void Client_GNS::Stop()
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        if ( m_connectionHandle != k_HSteamNetConnection_Invalid )
        {
            pInterface->CloseConnection( m_connectionHandle, k_ESteamNetConnectionEnd_App_Generic, "Closing Connection", false );
            m_connectionHandle = k_HSteamNetConnection_Invalid;
        }

        m_status = Status::Disconnected;
    }

    void Client_GNS::ConnectionChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo )
    {
        auto pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        Client_GNS* pClient = (Client_GNS*) pInterface->GetConnectionUserData( pInfo->m_hConn );

        // Handle state change
        //-------------------------------------------------------------------------

        // What's the state of the connection?
        switch ( pInfo->m_info.m_eState )
        {
            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            {
                // Clean up the connection.  This is important!
                // The connection is "closed" in the network sense, but
                // it has not been destroyed.  We must close it on our end, too
                // to finish up.  The reason information do not matter in this case,
                // and we cannot linger because it's already closed on the other end,
                // so we just pass 0's.
                pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
                pClient->m_connectionHandle = k_HSteamNetConnection_Invalid;

                // Try restore connection
                pClient->m_status = Client_GNS::Status::Reconnecting;
                break;
            }

            case k_ESteamNetworkingConnectionState_Connecting:
            case k_ESteamNetworkingConnectionState_FindingRoute:
            {
                pClient->m_status = Status::Connecting;
            }
            break;

            case k_ESteamNetworkingConnectionState_Connected:
            {
                pClient->m_reconnectionAttemptsRemaining = s_numReconnectionAttempts;
                pClient->m_status = Status::Connected;
            }
            break;

            case k_ESteamNetworkingConnectionState_None:
            // NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
            break;

            default:
            // Silences -Wswitch
            break;
        }
    }

    void Client_GNS::UpdateConnectionAndReceiveMessages()
    {
        EE_ASSERT( Threading::IsMainThread() );

        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );
        pInterface->RunCallbacks();

        // Handle Reconnection Attempts
        //-------------------------------------------------------------------------

        if ( m_status == Status::Reconnecting )
        {
            if ( m_reconnectionAttemptsRemaining > 0 )
            {
                m_reconnectionAttemptsRemaining--;
                Threading::Sleep( s_reconnectionAttemptCooldownMilliseconds );
                if ( !TryConnect() )
                {
                    m_status = Status::Reconnecting;
                    return;
                }
            }
            else
            {
                m_status = Status::ConnectionFailed;
                return;
            }
        }

        if ( !IsConnected() )
        {
            return;
        }

        // Receive
        //-------------------------------------------------------------------------

        TInlineVector<ISteamNetworkingMessage*, 20> incomingMessages;
        incomingMessages.resize( 20, nullptr );

        int32_t const numMsgs = pInterface->ReceiveMessagesOnConnection( m_connectionHandle, incomingMessages.data(), (int32_t) incomingMessages.size() );

        // Handle invalid connection handle
        if ( numMsgs < 0 )
        {
            EE_LOG_FATAL_ERROR( LogCategory::Network, "Client Connection", "Client connection handle is invalid, we've likely lost connection to the server" );
            return;
        }

        // Process received messages
        for ( int32 i = 0; i < numMsgs; i++ )
        {
            auto pMessage = EE::New<Message>( (char const*) incomingMessages[i]->GetData(), incomingMessages[i]->GetSize() );
            bool const wasEnqueued = m_receivedMessages.enqueue( pMessage );
            EE_ASSERT( wasEnqueued );
            incomingMessages[i]->Release();
        }
    }

    void Client_GNS::SendOutgoingMessages()
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        for ( auto& msg : m_outgoingMessages )
        {
            pInterface->SendMessageToConnection( m_connectionHandle, msg.GetTransmissionData(), (uint32_t) msg.GetTransmissionSize(), k_nSteamNetworkingSend_Reliable, nullptr );
        }

        m_outgoingMessages.clear();
    }
}