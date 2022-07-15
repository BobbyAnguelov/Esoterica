#include "IPCMessage.h"

//-------------------------------------------------------------------------

namespace EE::Network::IPC
{
    void Message::Initialize( MessageID messageID )
    {
        m_data.resize( sizeof( MessageID ) );
        *(MessageID*) m_data.data() = messageID;
    }

    void Message::Initialize( void* pData, size_t dataSize )
    {
        EE_ASSERT( pData != nullptr && dataSize >= sizeof( MessageID ) );
        m_data.resize( dataSize );
        memcpy( m_data.data(), pData, dataSize );
        EE_ASSERT( GetMessageID() != InvalidID );
    }

    void Message::Initialize( MessageID messageID, void* pData, size_t dataSize )
    {
        EE_ASSERT( messageID != InvalidID && pData != nullptr && dataSize > 0 );
        m_data.resize( dataSize + sizeof( MessageID ) );
        *(MessageID*) m_data.data() = messageID;
        memcpy( m_data.data() + sizeof( MessageID ), pData, dataSize );
    }
}