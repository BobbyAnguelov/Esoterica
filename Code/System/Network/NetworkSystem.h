#pragma once
#include "System/_Module/API.h"
#include "System/Types/Arrays.h"
#include "System/Types/Function.h"
#include "System/Types/String.h"

//-------------------------------------------------------------------------

namespace EE::Network
{
    class NetworkSystem;

    //-------------------------------------------------------------------------

    using AddressString = TInlineString<30>;

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API ServerConnection
    {
        friend NetworkSystem;
        friend struct NetworkCallbackHandler;

    public:

        using ClientConnectionID = uint32_t;

        struct ClientInfo
        {
            ClientInfo( ClientConnectionID ID, AddressString const& address )
                : m_ID( ID )
                , m_address( address )
            {
                EE_ASSERT( m_ID != 0 && !m_address.empty() );
            }

            ClientConnectionID      m_ID;
            AddressString           m_address;
        };

    public:

        virtual ~ServerConnection();

        // Server Info
        //-------------------------------------------------------------------------

        inline bool IsRunning() const { return m_socketHandle != 0 && m_pollingGroupHandle != 0; }
        inline uint32_t GetSocketHandle() const { return m_socketHandle; }

        // Client info
        //-------------------------------------------------------------------------

        inline int32_t GetNumConnectedClients() const { return (int32_t) m_connectedClients.size(); }
        inline TVector<ClientInfo> const& GetConnectedClients() const { return m_connectedClients; }

        inline ClientInfo const& GetConnectedClientInfo( int32_t clientIdx ) const 
        {
            EE_ASSERT( clientIdx >= 0 && clientIdx < m_connectedClients.size() );
            return m_connectedClients[clientIdx]; 
        }

        inline bool HasConnectedClient( ClientConnectionID clientID ) const 
        {
            auto const SearchPredicate = [] ( ClientInfo const& info, ClientConnectionID const& clientID ) { return info.m_ID == clientID; };
            return eastl::find( m_connectedClients.begin(), m_connectedClients.end(), clientID, SearchPredicate ) != m_connectedClients.end();
        }

        // Messages
        //-------------------------------------------------------------------------

        virtual void ProcessMessage( uint32_t connectionID, void* pData, size_t size ) = 0;
        virtual void SendMessages( TFunction<void( ClientConnectionID, void*, uint32_t )> const& sendFunction ) = 0;

    private:

        bool TryStartConnection( uint16_t portNumber );
        void CloseConnection();

        void AddConnectedClient( ClientConnectionID clientID, AddressString const& clientAddress );
        void RemoveConnectedClient( ClientConnectionID clientID );

    protected:

        uint32_t                                          m_socketHandle = 0;
        uint32_t                                          m_pollingGroupHandle = 0;
        TVector<ClientInfo>                             m_connectedClients;
    };

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API ClientConnection
    {
        friend NetworkSystem;
        friend struct NetworkCallbackHandler;

    public:

        enum class Status
        {
            Disconnected,
            Connecting,
            Connected,
            Reconnecting,
            ConnectionFailed
        };

    public:

        virtual ~ClientConnection();

        inline bool IsConnected() const { return m_status == Status::Connected; }
        inline bool IsConnecting() const { return m_status == Status::Connecting || m_status == Status::Reconnecting; };
        inline bool HasConnectionFailed() const { return m_status == Status::ConnectionFailed; }
        inline bool IsDisconnected() const { return m_status == Status::Disconnected; }

        inline uint32_t const& GetClientConnectionID() const { return m_connectionHandle; }
        inline AddressString const& GetAddress() const { return m_address; }

        virtual void ProcessMessage( void* pData, size_t size ) = 0;

        virtual void SendMessages( TFunction<void( void*, uint32_t )> const& sendFunction ) = 0;

    private:

        bool TryStartConnection();
        void CloseConnection();
        void Update();

    private:

        AddressString                                   m_address;
        uint32_t                                          m_connectionHandle = 0;
        uint32_t                                          m_reconnectionAttemptsRemaining = 5;
        Status                                          m_status = Status::Disconnected;
    };

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API NetworkSystem final
    {
        friend struct NetworkCallbackHandler;

    public:

        static bool Initialize();
        static void Shutdown();
        static void Update();

        // Start a server connection on the specified port
        static bool StartServerConnection( ServerConnection* pServerConnection, uint16_t portNumber );
        static void StopServerConnection( ServerConnection* pServerConnection );

        // Start a client connection to a specified address. Address format: "XXX.XXX.XXX.XXX:Port"
        static bool StartClientConnection( ClientConnection* pClientConnection, char const* pAddress );
        static void StopClientConnection( ClientConnection* pClientConnection );

    private:

        NetworkSystem() = delete;
    };
}