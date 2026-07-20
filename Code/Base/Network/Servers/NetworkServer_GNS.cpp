#include "NetworkServer_GNS.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

//-------------------------------------------------------------------------

namespace EE::Network
{
    namespace
    {
        struct ServerLUT
        {
            ServerLUT() = default;
            ServerLUT( uint32_t hSocket, Server_GNS* pServer ) : m_hSocket( hSocket ), m_pServer( pServer ) {}

        public:

            uint32_t m_hSocket = 0;
            Server_GNS* m_pServer = nullptr;
        };

        TInlineVector<ServerLUT, 10> g_registeredServers;

        //-------------------------------------------------------------------------

        static void RegisterServer( uint32_t hSocket, Server_GNS* pServer )
        {
            EE_ASSERT( hSocket != 0 && pServer != nullptr );

            for ( int32 i = 0; i < g_registeredServers.size(); i++ )
            {
                if ( g_registeredServers[i].m_hSocket == hSocket )
                {
                    EE_HALT();
                }
            }

            g_registeredServers.emplace_back( hSocket, pServer );
        }

        static void UnregisterServer( uint32_t hSocket )
        {
            EE_ASSERT( hSocket != 0 );

            for ( int32 i = 0; i < g_registeredServers.size(); i++ )
            {
                if ( g_registeredServers[i].m_hSocket == hSocket )
                {
                    g_registeredServers.erase( g_registeredServers.begin() + i );
                    break;
                }
            }

            if ( g_registeredServers.empty() )
            {
                g_registeredServers.clear( true );
            }
        }

        static Server_GNS* GetRegisteredServer( uint32_t hSocket )
        {
            EE_ASSERT( hSocket != 0 );

            for ( int32 i = 0; i < g_registeredServers.size(); i++ )
            {
                if ( g_registeredServers[i].m_hSocket == hSocket )
                {
                    return g_registeredServers[i].m_pServer;
                }
            }

            return nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void Server_GNS::ConnectionChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo )
    {
        auto pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        Server_GNS* pServer = GetRegisteredServer( pInfo->m_info.m_hListenSocket );
        if ( pServer == nullptr )
        {
            return;
        }

        // Handle state change
        //-------------------------------------------------------------------------

        switch ( pInfo->m_info.m_eState )
        {
            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            {
                // Ignore if they were not previously connected.  (If they disconnected before we accepted the connection.)
                if ( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected )
                {
                    // Note that clients should have been found, because this is the only code path where we remove clients( except on shutdown ), and connection change callbacks are dispatched in queue order.
                    EE_ASSERT( pServer->HasConnectedClient( pInfo->m_hConn ) );
                    pServer->DisconnectClient( pInfo->m_hConn );

                }
                else
                {
                    EE_ASSERT( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting );
                }

                // Clean up the connection.  This is important!
                // The connection is "closed" in the network sense, but
                // it has not been destroyed.  We must close it on our end, too
                // to finish up.  The reason information do not matter in this case,
                // and we cannot linger because it's already closed on the other end,
                // so we just pass 0's.
                pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
                break;
            }

            case k_ESteamNetworkingConnectionState_Connecting:
            {
                // This must be a new connection
                EE_ASSERT( !pServer->HasConnectedClient( pInfo->m_hConn ) );

                // A client is attempting to connect
                // Try to accept the connection.
                if ( pInterface->AcceptConnection( pInfo->m_hConn ) != k_EResultOK )
                {
                    // This could fail. If the remote host tried to connect, but then
                    // disconnected, the connection may already be half closed.  Just
                    // destroy whatever we have on our side.
                    pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
                    break;
                }

                // Assign the poll group
                if ( !pInterface->SetConnectionPollGroup( pInfo->m_hConn, pServer->m_pollingGroupHandle ) )
                {
                    pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
                    break;
                }

                InlineString clientAddress( InlineString::CtorDoNotInitialize(), InlineString::kMaxSize );
                pInfo->m_info.m_addrRemote.ToString( clientAddress.data(), InlineString::kMaxSize, true );

                pServer->ConnectClient( (uint64_t) pInfo->m_hConn, clientAddress.c_str(), nullptr );
            }
            break;

            case k_ESteamNetworkingConnectionState_None:
            // NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
            break;

            case k_ESteamNetworkingConnectionState_Connected:
            // We will get a callback immediately after accepting the connection.
            // Since we are the server, we can ignore this, it's not news to us.
            break;

            default:
            // Silences -Wswitch
            break;
        }
    }

    //-------------------------------------------------------------------------

    Server_GNS::~Server_GNS()
    {
        EE_ASSERT( m_pollingGroupHandle == k_HSteamNetPollGroup_Invalid && m_socketHandle == k_HSteamListenSocket_Invalid );
    }

    bool Server_GNS::Start( uint16_t port )
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        //-------------------------------------------------------------------------

        m_port = port;

        SteamNetworkingIPAddr serverLocalAddr;
        serverLocalAddr.Clear();
        serverLocalAddr.m_port = port;

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*) ConnectionChangedCallback );
        m_socketHandle = pInterface->CreateListenSocketIP( serverLocalAddr, 1, &opt );
        if ( m_socketHandle == k_HSteamListenSocket_Invalid )
        {
            EE_LOG_ERROR( LogCategory::Network, "Server Connection", "Failed to listen on port %d", m_port );
            return false;
        }

        m_pollingGroupHandle = pInterface->CreatePollGroup();
        if ( m_pollingGroupHandle == k_HSteamNetPollGroup_Invalid )
        {
            EE_LOG_ERROR( LogCategory::Network, "Server Connection", "Failed to listen on port %d", m_port );
            return false;
        }

        RegisterServer( m_socketHandle, this );

        return true;
    }

    void Server_GNS::Stop()
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        //-------------------------------------------------------------------------

        for ( auto const& clientInfo : m_connectedClients )
        {
            pInterface->CloseConnection( (uint32_t) clientInfo.m_ID, 0, "Server Shutdown", true );
        }
        m_connectedClients.clear();

        //-------------------------------------------------------------------------

        UnregisterServer( m_socketHandle );

        if ( m_pollingGroupHandle != k_HSteamNetPollGroup_Invalid )
        {
            pInterface->DestroyPollGroup( m_pollingGroupHandle );
            m_pollingGroupHandle = k_HSteamNetPollGroup_Invalid;
        }

        if ( m_socketHandle != k_HSteamListenSocket_Invalid )
        {
            pInterface->CloseListenSocket( m_socketHandle );
            m_socketHandle = k_HSteamListenSocket_Invalid;
        }
    }

    void Server_GNS::UpdateConnectionAndReceiveMessages()
    {
        EE_ASSERT( Threading::IsMainThread() );

        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );
        pInterface->RunCallbacks();

        TInlineVector<ISteamNetworkingMessage*, 20> incomingMessages;
        incomingMessages.resize( 20, nullptr );

        ISteamNetworkingMessage *pIncomingMsg = nullptr;
        int32_t const numMsgs = pInterface->ReceiveMessagesOnPollGroup( m_pollingGroupHandle, incomingMessages.data(), (int32_t) incomingMessages.size() );

        if ( numMsgs < 0 )
        {
            EE_LOG_ERROR( LogCategory::Network, "Server Connection", "Failed to check for messages!Something has gone wrong with the server connection!" );
            return;
        }

        for ( int32 i = 0; i < numMsgs; i++ )
        {
            auto pMessage = EE::New<Message>( (char const*) incomingMessages[i]->GetData(), incomingMessages[i]->GetSize() );
            pMessage->SetClientID( incomingMessages[i]->m_conn );
            bool const wasEnqueued = m_receivedMessages.enqueue( pMessage );
            EE_ASSERT( wasEnqueued );
            incomingMessages[i]->Release();
        }
    }

    void Server_GNS::SendOutgoingMessages()
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        for ( auto& msg : m_outgoingMessages )
        {
            if ( msg.GetClientID() != 0 )
            {
                pInterface->SendMessageToConnection( (uint32_t) msg.GetClientID(), msg.GetTransmissionData(), (uint32_t) msg.GetTransmissionSize(), k_nSteamNetworkingSend_Reliable, nullptr );
            }
            else // Send to All
            {
                for ( auto const& client : m_connectedClients )
                {
                    pInterface->SendMessageToConnection( (uint32_t) client.m_ID, msg.GetTransmissionData(), (uint32_t) msg.GetTransmissionSize(), k_nSteamNetworkingSend_Reliable, nullptr );
                }
            }
        }

        m_outgoingMessages.clear();
    }
}