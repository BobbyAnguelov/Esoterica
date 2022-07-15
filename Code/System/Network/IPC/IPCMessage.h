#pragma once
#include "System/_Module/API.h"
#include "System/Types/Arrays.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Network::IPC
{
    enum class MessageType
    {
        Generic = 0,
        NumMessageTypes
    };

    //-------------------------------------------------------------------------

    typedef int32_t MessageID;

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API Message
    {
        friend class Server;
        friend class Client;
        constexpr static MessageID const InvalidID = 0xFFFFFFFF;

    public:

        Message() { Initialize( InvalidID ); }
        explicit Message( MessageID ID ) { Initialize( ID ); }
        explicit Message( MessageID ID, void* pData, size_t dataSize ) { Initialize( ID, pData, dataSize ); }

        explicit Message( Message&& msg ) { m_clientConnectionID = msg.m_clientConnectionID; m_data.swap( msg.m_data ); }
        explicit Message( Message const& msg ) { m_clientConnectionID = msg.m_clientConnectionID; m_data = msg.m_data; }

        template<typename T>
        Message( MessageID ID, T&& typeToSerialize )
        {
            Serialization::BinaryOutputArchive archive;
            archive << typeToSerialize;
            Initialize( ID, (void*) archive.GetBinaryData(), archive.GetBinaryDataSize() );
        }

        //-------------------------------------------------------------------------

        Message& operator=( Message const& msg )
        {
            m_data = msg.m_data;
            return *this;
        }

        Message& operator=( Message&& msg ) noexcept
        {
            m_data.swap( msg.m_data );
            return *this;
        }

        //-------------------------------------------------------------------------

        inline uint32_t const& GetClientConnectionID() const { return m_clientConnectionID; }
        inline void SetClientConnectionID( uint32_t clientConnectionID ) { m_clientConnectionID = clientConnectionID; }
        inline MessageID GetMessageID() const { return *(MessageID*) m_data.data(); }

        inline bool IsValid() const { return GetMessageID() != InvalidID; }
        inline bool HasPayload() const { return GetPayloadDataSize() > sizeof( MessageID ); }
        inline uint8_t const* GetPayloadData() const { return m_data.data() + sizeof( MessageID ); }
        inline size_t GetPayloadDataSize() const { return m_data.size() - sizeof( MessageID ); }

        // Serialization functions
        template<typename T>
        inline void SetData( MessageID msgID, T const& typeToSerialize )
        {
            Serialization::BinaryOutputArchive archive;
            archive << typeToSerialize;
            Initialize( msgID, (void*) archive.GetBinaryData(), archive.GetBinaryDataSize() );
        }

        // Serialization functions
        template<typename T>
        inline T GetData() const
        {
            T outType;
            Serialization::BinaryInputArchive archive;
            archive.ReadFromData( GetPayloadData(), GetPayloadDataSize() );
            archive << outType;
            return outType;
        }

    private:

        void Initialize( MessageID messageID );
        void Initialize( void* pData, size_t dataSize );
        void Initialize( MessageID messageID, void* pData, size_t dataSize );

    private:

        TInlineVector<uint8_t, sizeof( MessageID )>     m_data;
        uint32_t                                        m_clientConnectionID = 0;
    };
}