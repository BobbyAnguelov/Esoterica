#include "NetworkSystem.h"
#include "System/Threading/Threading.h"
#include "System/Log.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

//-------------------------------------------------------------------------

namespace EE::Network
{
    struct NetworkState
    {
        int64_t                                   m_logTimeZero = 0;
        TInlineVector<ServerConnection*, 2>     m_serverConnections;
        TInlineVector<ClientConnection*, 2>     m_clientConnections;
    };

    static NetworkState* g_pNetworkState = nullptr;

    //-------------------------------------------------------------------------

    struct NetworkCallbackHandler
    {
        static void NetworkDebugOutputFunction( ESteamNetworkingSocketsDebugOutputType type, char const* pMessage )
        {
            EE_ASSERT( g_pNetworkState != nullptr );
            SteamNetworkingMicroseconds const currentTime = SteamNetworkingUtils()->GetLocalTimestamp() - g_pNetworkState->m_logTimeZero;

            EE_TRACE_MSG( pMessage );

            // TODO: initialize rpmalloc for the steam network thread
            if ( type == k_ESteamNetworkingSocketsDebugOutputType_Bug )
            {
                //EE_LOG_FATAL_ERROR( "Network", "(%10.6f) %s", currentTime * 1e-6, pMessage );
            }
            else if ( type == k_ESteamNetworkingSocketsDebugOutputType_Error )
            {
                //EE_LOG_ERROR( "Network", "(%10.6f) %s", currentTime * 1e-6, pMessage );
            }
            else if ( type == k_ESteamNetworkingSocketsDebugOutputType_Warning )
            {
                //EE_LOG_WARNING( "Network", "(%10.6f) %s", currentTime * 1e-6, pMessage );
            }
            else if ( type == k_ESteamNetworkingSocketsDebugOutputType_Important )
            {
                //EE_LOG_MESSAGE( "Network", "(%10.6f) %s", currentTime * 1e-6, pMessage );
            }
            else // Verbose
            {
                //EE_LOG_MESSAGE( "Network", "(%10.6f) %s", currentTime * 1e-6, pMessage );
            }
        }

        static void ServerNetConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo )
        {
            auto pInterface = SteamNetworkingSockets();
            EE_ASSERT( pInterface != nullptr );

            // Find Server Connection
            //-------------------------------------------------------------------------

            auto FindServerConnection = [] ( ServerConnection const* pConnection, uint32_t socketHandle )
            {
                return pConnection->GetSocketHandle() == socketHandle;
            };

            auto serverConnectionIter = eastl::find( g_pNetworkState->m_serverConnections.begin(), g_pNetworkState->m_serverConnections.end(), pInfo->m_info.m_hListenSocket, FindServerConnection );
            EE_ASSERT( serverConnectionIter != g_pNetworkState->m_serverConnections.end() );
            ServerConnection* pServerConnection = *serverConnectionIter;

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
                        EE_ASSERT( pServerConnection->HasConnectedClient( pInfo->m_hConn ) );
                        pServerConnection->RemoveConnectedClient( pInfo->m_hConn );
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
                    EE_ASSERT( !pServerConnection->HasConnectedClient( pInfo->m_hConn ) );

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
                    if ( !pInterface->SetConnectionPollGroup( pInfo->m_hConn, pServerConnection->m_pollingGroupHandle ) )
                    {
                        pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
                        break;
                    }

                    AddressString clientAddress( AddressString::CtorDoNotInitialize(), AddressString::kMaxSize );
                    pInfo->m_info.m_addrRemote.ToString( clientAddress.data(), AddressString::kMaxSize, true );
                    pServerConnection->AddConnectedClient( pInfo->m_hConn, clientAddress );
                    break;
                }

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

        static void ClientNetConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo )
        {
            auto pInterface = SteamNetworkingSockets();
            EE_ASSERT( pInterface != nullptr );

            // Find Client Connection
            //-------------------------------------------------------------------------

            auto FindClientConnection = [] ( ClientConnection const* pConnection, uint32_t connectionHandle )
            {
                return pConnection->GetClientConnectionID() == connectionHandle;
            };

            auto clientConnectionIter = eastl::find( g_pNetworkState->m_clientConnections.begin(), g_pNetworkState->m_clientConnections.end(), pInfo->m_hConn, FindClientConnection );
            
            // Ignore status changes messages for already closed connections
            if ( clientConnectionIter == g_pNetworkState->m_clientConnections.end() )
            {
                return;
            }

            ClientConnection* pClientConnection = *clientConnectionIter;

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
                    pClientConnection->m_connectionHandle = k_HSteamNetConnection_Invalid;

                    // Try restore connection
                    pClientConnection->m_status = ClientConnection::Status::Reconnecting;
                    break;
                }

                case k_ESteamNetworkingConnectionState_Connecting:
                // We will get this callback when we start connecting.
                // We can ignore this.
                break;

                case k_ESteamNetworkingConnectionState_Connected:
                break;

                case k_ESteamNetworkingConnectionState_None:
                // NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
                break;

                default:
                // Silences -Wswitch
                break;
            }
        }
    };

    //-------------------------------------------------------------------------

    ServerConnection::~ServerConnection()
    {
        EE_ASSERT( m_pollingGroupHandle == k_HSteamNetPollGroup_Invalid && m_socketHandle == k_HSteamListenSocket_Invalid );
    }

    void ServerConnection::AddConnectedClient( ClientConnectionID clientHandle, AddressString const& clientAddress )
    {
        EE_ASSERT( !HasConnectedClient( clientHandle ) );
        m_connectedClients.emplace_back( clientHandle, clientAddress );
    }

    void ServerConnection::RemoveConnectedClient( ClientConnectionID clientHandle )
    {
        auto const SearchPredicate = [] ( ClientInfo const& info, ClientConnectionID const& clientHandle ) { return info.m_ID == clientHandle; };
        auto clientIter = eastl::find( m_connectedClients.begin(), m_connectedClients.end(), clientHandle, SearchPredicate );
        EE_ASSERT( clientIter != m_connectedClients.end() );
        m_connectedClients.erase_unsorted( clientIter );
    }

    bool ServerConnection::TryStartConnection( uint16_t portNumber )
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        //-------------------------------------------------------------------------

        SteamNetworkingIPAddr serverLocalAddr;
        serverLocalAddr.Clear();
        serverLocalAddr.m_port = portNumber;

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*) NetworkCallbackHandler::ServerNetConnectionStatusChangedCallback );
        m_socketHandle = pInterface->CreateListenSocketIP( serverLocalAddr, 1, &opt );
        if ( m_socketHandle == k_HSteamListenSocket_Invalid )
        {
            EE_LOG_ERROR( "Network", "Server Connection", "Failed to listen on port %d", portNumber );
            return false;
        }

        m_pollingGroupHandle = pInterface->CreatePollGroup();
        if ( m_pollingGroupHandle == k_HSteamNetPollGroup_Invalid )
        {
            EE_LOG_ERROR( "Network", "Server Connection", "Failed to listen on port %d", portNumber );
            return false;
        }

        return true;
    }

    void ServerConnection::CloseConnection()
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        //-------------------------------------------------------------------------

        for ( auto const& clientInfo : m_connectedClients )
        {
            pInterface->CloseConnection( clientInfo.m_ID, 0, "Server Shutdown", true );
        }
        m_connectedClients.clear();

        //-------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------

    ClientConnection::~ClientConnection()
    {
        EE_ASSERT( m_connectionHandle == k_HSteamNetConnection_Invalid );
    }

    void ClientConnection::Update()
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        //-------------------------------------------------------------------------

        if ( m_status == Status::Reconnecting )
        {
            if ( m_reconnectionAttemptsRemaining > 0 )
            {
                m_reconnectionAttemptsRemaining--;
                Threading::Sleep( 100 );
                if ( !TryStartConnection() )
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

        //-------------------------------------------------------------------------

        EE_ASSERT( m_connectionHandle != k_HSteamNetConnection_Invalid );
        SteamNetConnectionInfo_t info;
        pInterface->GetConnectionInfo( m_connectionHandle, &info );

        switch ( info.m_eState )
        {
            case k_ESteamNetworkingConnectionState_Connecting:
            case k_ESteamNetworkingConnectionState_FindingRoute:
            {
                m_status = Status::Connecting;
            }
            break;

            case k_ESteamNetworkingConnectionState_Connected:
            {
                m_reconnectionAttemptsRemaining = 5;
                m_status = Status::Connected;
            }
            break;

            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }
    }

    bool ClientConnection::TryStartConnection()
    {
        SteamNetworkingIPAddr serverAddr;
        if ( !serverAddr.ParseString( m_address.c_str() ) )
        {
            EE_LOG_ERROR( "Network", "Client Connection", "Invalid client IP address provided: %s", m_address.c_str() );
            return false;
        }

        if ( serverAddr.m_port == 0 )
        {
            EE_LOG_ERROR( "Network", "Client Connection", "No port for client address provided: %s", m_address.c_str() );
            return false;
        }

        //-------------------------------------------------------------------------

        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*) NetworkCallbackHandler::ClientNetConnectionStatusChangedCallback );
        m_connectionHandle = pInterface->ConnectByIPAddress( serverAddr, 1, &opt );
        if ( m_connectionHandle == k_HSteamNetConnection_Invalid )
        {
            EE_LOG_ERROR( "Network", "Client Connection", "Failed to create connection" );
            return false;
        }

        m_status = Status::Connecting;
        return true;
    }

    void ClientConnection::CloseConnection()
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

    //-------------------------------------------------------------------------

    bool NetworkSystem::Initialize()
    {
        EE_ASSERT( g_pNetworkState == nullptr );

        SteamDatagramErrMsg errMsg;
        if ( !GameNetworkingSockets_Init( nullptr, errMsg ) )
        {
            EE_LOG_ERROR( "Network", nullptr, "Failed to initialize network system: %s", errMsg );
            return false;
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        SteamNetworkingUtils()->SetDebugOutputFunction( k_ESteamNetworkingSocketsDebugOutputType_Msg, NetworkCallbackHandler::NetworkDebugOutputFunction );
        #endif

        g_pNetworkState = EE::New<NetworkState>();
        g_pNetworkState->m_logTimeZero = SteamNetworkingUtils()->GetLocalTimestamp();
        return true;
    }

    void NetworkSystem::Shutdown()
    {
        if ( g_pNetworkState != nullptr )
        {
            // Give connections time to finish up.  This is an application layer protocol here, it's not TCP.  Note that if you have an application and you need to be
            // more sure about cleanup, you won't be able to do this.  You will need to send a message and then either wait for the peer to close the connection, or
            // you can pool the connection to see if any reliable data is pending.
            Threading::Sleep( 250 );

            GameNetworkingSockets_Kill();
            EE::Delete( g_pNetworkState );
        }
    }

    void NetworkSystem::Update()
    {
        auto pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        //-------------------------------------------------------------------------
        // Handle connection changes
        //-------------------------------------------------------------------------

        pInterface->RunCallbacks();

        //-------------------------------------------------------------------------
        // Servers
        //-------------------------------------------------------------------------

        for ( auto pServerConnection : g_pNetworkState->m_serverConnections )
        {
            // Receive
            //-------------------------------------------------------------------------

            ISteamNetworkingMessage *pIncomingMsg = nullptr;
            int32_t const numMsgs = pInterface->ReceiveMessagesOnPollGroup( pServerConnection->m_pollingGroupHandle, &pIncomingMsg, 1 );

            if ( numMsgs < 0 )
            {
                //EE_LOG_FATAL_ERROR( "Network", "Error checking for messages" );
                break;
            }

            if ( numMsgs > 0 )
            {
                pServerConnection->ProcessMessage( pIncomingMsg->m_conn, pIncomingMsg->m_pData, pIncomingMsg->m_cbSize );
                pIncomingMsg->Release();
            }

            // Send
            //-------------------------------------------------------------------------

            auto ServerSendFunction = [pInterface, pServerConnection] ( uint32_t connectionHandle, void* pData, uint32_t size )
            {
                pInterface->SendMessageToConnection( connectionHandle, pData, size, k_nSteamNetworkingSend_Reliable, nullptr );
            };

            pServerConnection->SendMessages( ServerSendFunction );
        }

        //-------------------------------------------------------------------------
        // Clients
        //-------------------------------------------------------------------------

        for ( auto pClientConnection : g_pNetworkState->m_clientConnections )
        {
            pClientConnection->Update();

            if ( pClientConnection->IsConnected() )
            {
                // Receive
                //-------------------------------------------------------------------------

                ISteamNetworkingMessage* pIncomingMsg = nullptr;
                int32_t const numMsgs = pInterface->ReceiveMessagesOnConnection( pClientConnection->m_connectionHandle, &pIncomingMsg, 1 );

                // Handle invalid connection handle
                if ( numMsgs < 0 )
                {
                    //EE_LOG_FATAL_ERROR( "Network", "Client connection handle is invalid, we've likely lost connection to the server" );
                    break;
                }

                // Process received messages
                if ( numMsgs > 0 )
                {
                    pClientConnection->ProcessMessage( pIncomingMsg->m_pData, pIncomingMsg->m_cbSize );
                    pIncomingMsg->Release();
                }

                // Send
                //-------------------------------------------------------------------------

                auto ClientSendFunction = [pInterface, pClientConnection] ( void* pData, uint32_t size )
                {
                    pInterface->SendMessageToConnection( pClientConnection->m_connectionHandle, pData, size, k_nSteamNetworkingSend_Reliable, nullptr );
                };

                pClientConnection->SendMessages( ClientSendFunction );
            }
        }
    }

    //-------------------------------------------------------------------------

    bool NetworkSystem::StartServerConnection( ServerConnection* pServerConnection, uint16_t portNumber )
    {
        EE_ASSERT( pServerConnection != nullptr && !pServerConnection->IsRunning() );
        if ( pServerConnection->TryStartConnection( portNumber ) )
        {
            g_pNetworkState->m_serverConnections.emplace_back( pServerConnection );
            return true;
        }

        return false;
    }

    void NetworkSystem::StopServerConnection( ServerConnection* pServerConnection )
    {
        EE_ASSERT( pServerConnection != nullptr );
        pServerConnection->CloseConnection();
        g_pNetworkState->m_serverConnections.erase_first_unsorted( pServerConnection );
    }

    //-------------------------------------------------------------------------

    bool NetworkSystem::StartClientConnection( ClientConnection* pClientConnection, char const* pAddress )
    {
        EE_ASSERT( pClientConnection != nullptr && pClientConnection->IsDisconnected() );
        EE_ASSERT( pAddress != nullptr );

        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        EE_ASSERT( pInterface != nullptr );

        //-------------------------------------------------------------------------

        pClientConnection->m_address = pAddress;
        if ( !pClientConnection->TryStartConnection() )
        {
            return false;
        }

        g_pNetworkState->m_clientConnections.emplace_back( pClientConnection );
        return true;
    }

    void NetworkSystem::StopClientConnection( ClientConnection* pClientConnection )
    {
        EE_ASSERT( pClientConnection != nullptr && !pClientConnection->IsDisconnected() );

        pClientConnection->CloseConnection();
        pClientConnection->m_address.clear();
        g_pNetworkState->m_clientConnections.erase_first_unsorted( pClientConnection );
    }
}