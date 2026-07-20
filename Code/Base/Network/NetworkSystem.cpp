#include "NetworkSystem.h"
#include "Servers/NetworkServer.h"
#include "Clients/NetworkClient.h"
#include "Base/Threading/Threading.h"
#include "Base/Memory/Memory.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#if EE_DEVELOPMENT_TOOLS
#include <ixwebsocket/IXNetSystem.h>
#endif

//-------------------------------------------------------------------------

namespace EE::Network
{
    static void NetworkDebugOutputFunction( ESteamNetworkingSocketsDebugOutputType type, char const* pMessage )
    {
        EE_TRACE_MSG( pMessage );

        // TODO: initialize rpmalloc for the steam network thread
        if ( type == k_ESteamNetworkingSocketsDebugOutputType_Bug )
        {
            //EE_LOG_FATAL_ERROR( Log::Network, "NetworkSystem", "(%10.6f) %s", currentTime * 1e-6, pMessage );
        }
        else if ( type == k_ESteamNetworkingSocketsDebugOutputType_Error )
        {
            //EE_LOG_ERROR( Log::Network, "NetworkSystem", "(%10.6f) %s", currentTime * 1e-6, pMessage );
        }
        else if ( type == k_ESteamNetworkingSocketsDebugOutputType_Warning )
        {
            //EE_LOG_WARNING( Log::Network, "NetworkSystem", "(%10.6f) %s", currentTime * 1e-6, pMessage );
        }
        else if ( type == k_ESteamNetworkingSocketsDebugOutputType_Important )
        {
            //EE_LOG_MESSAGE( Log::Network, "NetworkSystem", "(%10.6f) %s", currentTime * 1e-6, pMessage );
        }
        else // Verbose
        {
            //EE_LOG_MESSAGE( Log::Network, "NetworkSystem", "(%10.6f) %s", currentTime * 1e-6, pMessage );
        }
    }

    //-------------------------------------------------------------------------

    bool NetworkSystem::Initialize()
    {
        // GNS
        //-------------------------------------------------------------------------

        SteamDatagramErrMsg errMsg;
        if ( !GameNetworkingSockets_Init( nullptr, errMsg ) )
        {
            EE_LOG_ERROR( LogCategory::Network, nullptr, "Failed to initialize network system: %s", errMsg );
            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        SteamNetworkingUtils()->SetDebugOutputFunction( k_ESteamNetworkingSocketsDebugOutputType_Msg, NetworkDebugOutputFunction );
        #endif

        // Web Sockets
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( !ix::initNetSystem() )
        {
            return false;
        }
        #endif

        return true;
    }

    void NetworkSystem::Shutdown()
    {
        // Web Sockets
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        ix::uninitNetSystem();
        #endif

        // GNS
        //-------------------------------------------------------------------------

        // Give connections time to finish up. This is an application layer protocol here, it's not TCP.  Note that if you have an application and you need to be
        // more sure about cleanup, you won't be able to do this.  You will need to send a message and then either wait for the peer to close the connection, or
        // you can pool the connection to see if any reliable data is pending.
        Threading::Sleep( 250 );

        GameNetworkingSockets_Kill();
    }
}