#pragma once
#include "Base/_Module/API.h"
#include "Base/Types/Arrays.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------
// Binary serialized network message using msgpack
//-------------------------------------------------------------------------
// messages take the form of:
//
//     int32_t      message ID
//     byte *       payload ( msgpack serialized type )

namespace EE::Network
{
    using MessageID = int32_t;

    //-------------------------------------------------------------------------

    class EE_BASE_API Message
    {
    public:

        constexpr static MessageID const InvalidID = -1;
        constexpr static MessageID const HeartbeatID = -2;

    public:

        Message() { Initialize( InvalidID ); }
        explicit Message( MessageID ID ) { Initialize( ID ); }

        explicit Message( char const* pData, size_t dataSize ) { FromReceivedData( (void*) pData, dataSize ); }

        explicit Message( Message&& msg );
        explicit Message( Message const& msg );

        template<typename T>
        explicit Message( MessageID ID, T&& typeToSerialize )
        {
            Serialization::BinaryOutputArchive archive;
            archive << typeToSerialize;
            Initialize( ID, (void*) archive.GetBinaryData(), archive.GetBinaryDataSize() );
        }

        //-------------------------------------------------------------------------

        inline Message& operator=( Message const& msg )
        {
            m_data = msg.m_data;
            return *this;
        }

        inline Message& operator=( Message&& msg ) noexcept
        {
            m_data.swap( msg.m_data );
            return *this;
        }

        //-------------------------------------------------------------------------

        inline bool IsValid() const { return GetMessageID() != InvalidID; }

        //-------------------------------------------------------------------------

        // Set the client connection ID
        inline void SetClientID( uint64_t clientConnectionID ) { m_clientID = clientConnectionID; }

        // Get the client connection ID
        inline uint64_t const& GetClientID() const { return m_clientID; }

        //-------------------------------------------------------------------------

        inline MessageID GetMessageID() const { return *(MessageID*) m_data.data(); }

        inline bool HasPayload() const { return GetPayloadDataSize() > sizeof( MessageID ); }

        template<typename T>
        inline void SetPayload( MessageID msgID, T const& typeToSerialize )
        {
            Serialization::BinaryOutputArchive archive;
            archive << typeToSerialize;
            Initialize( msgID, (void*) archive.GetBinaryData(), archive.GetBinaryDataSize() );
        }

        template<typename T>
        inline T GetPayload() const
        {
            T outType;
            Serialization::BinaryInputArchive archive;
            archive.ReadFromData( GetPayloadDataPtr(), GetPayloadDataSize() );
            archive << outType;
            return outType;
        }

        inline char const* GetTransmissionData() const { return (char const*) m_data.data(); }

        inline size_t GetTransmissionSize() const { return m_data.size(); }

    private:

        void Initialize( MessageID messageID );
        void Initialize( MessageID messageID, void* pData, size_t dataSize );

        void FromReceivedData( void* pData, size_t dataSize );

        inline uint8_t const* GetPayloadDataPtr() const { return m_data.data() + sizeof( MessageID ); }
        inline size_t GetPayloadDataSize() const { return m_data.size() - sizeof( MessageID ); }

    private:

        TInlineVector<uint8_t, sizeof( MessageID )>     m_data;
        uint64_t                                        m_clientID = 0;
    };
}