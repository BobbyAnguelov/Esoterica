#pragma once

#include "System/_Module/API.h"
#include "System/Memory/Memory.h"
#include "System/ThirdParty/rapidjson/prettywriter.h"
#include "System/ThirdParty/rapidjson/document.h"

//-------------------------------------------------------------------------

namespace rapidjson
{
    enum ParseErrorCode;
    class CrtAllocator;
}

namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

namespace EE::Serialization
{
    struct RapidJsonAllocator
    {
        inline void* Malloc( size_t size ) { return EE::Alloc( size ); }
        inline void* Realloc( void* pOriginalPtr, size_t originalSize, size_t newSize ) { return EE::Realloc( pOriginalPtr, newSize ); }
        inline static void Free( void* pData ) { EE::Free( pData ); }
    };

    //-------------------------------------------------------------------------

    using JsonStringBuffer = rapidjson::GenericStringBuffer<rapidjson::UTF8<char>, RapidJsonAllocator>;
    using JsonWriter = rapidjson::PrettyWriter< JsonStringBuffer, rapidjson::UTF8<char>, rapidjson::UTF8<char>, rapidjson::CrtAllocator, 0>;
    using JsonValue = rapidjson::Value;

    //-------------------------------------------------------------------------

    EE_SYSTEM_API char const* GetJsonErrorMessage( rapidjson::ParseErrorCode code );

    // File reader
    //-------------------------------------------------------------------------

    class EE_SYSTEM_API JsonArchiveReader
    {
    public:

        virtual ~JsonArchiveReader();

        // Read entire json file
        bool ReadFromFile( FileSystem::Path const& filePath );

        // Read from a json string
        bool ReadFromString( char const* pString );

        // Get the read document
        inline rapidjson::Document const& GetDocument() const { return m_document; }

        // Get the read document
        inline rapidjson::Document& GetDocument() { return m_document; }

    protected:

        // Reset the reader state
        virtual void Reset();

        // Called if we successfully read and parsed the json data
        virtual void OnFileReadSuccess() {}

    protected:

        rapidjson::Document                                     m_document;

    private:

        uint8_t*                                                m_pStringBuffer = nullptr;
    };

    // JSON writer
    //-------------------------------------------------------------------------

    class EE_SYSTEM_API JsonArchiveWriter
    {
    public:

        virtual ~JsonArchiveWriter() = default;

        // Get the writer
        inline Serialization::JsonWriter* GetWriter() { return &m_writer; }

        // Get the generated string buffer
        inline JsonStringBuffer const& GetStringBuffer() const { return m_stringBuffer; }

        // Write currently serialized data to disk and reset serialized data
        bool WriteToFile( FileSystem::Path const& outPath );

        // Reset all serialized data without writing to disk
        virtual void Reset();

    protected:

        // Called as part of the write to file function to allow clients to finalize serialized data
        virtual void FinalizeSerializedData() {}

    protected:

        Serialization::JsonWriter                               m_writer = Serialization::JsonWriter( m_stringBuffer );
        JsonStringBuffer                                        m_stringBuffer;
    };
}