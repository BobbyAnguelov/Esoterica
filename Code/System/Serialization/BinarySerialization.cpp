#include "BinarySerialization.h"
#include "System/Types/String.h"
#include "System/Types/StringID.h"
#include "System/FileSystem/FileSystemPath.h"

#include "System/ThirdParty/mpack/mpack.h"

//-------------------------------------------------------------------------

namespace EE::Serialization
{
    int32_t GetBinarySerializationVersion()
    {
        return 5;
    }

    //-------------------------------------------------------------------------

    static void MPackReaderError( mpack_reader_t* pReader, mpack_error_t error )
    {
        EE_HALT();
    };

    BinaryReader::~BinaryReader()
    {
        Reset();
    }

    void BinaryReader::Reset()
    {
        if ( m_pReader != nullptr )
        {
            EndReading();
        }
    }

    void BinaryReader::BeginReading( char const* pData, size_t size )
    {
        EE_ASSERT( pData != nullptr );
        EE_ASSERT( m_pReader == nullptr );
        m_pReader = EE::New<mpack_reader_t>();
        mpack_reader_init_data( m_pReader, pData, size );
        mpack_reader_set_error_handler( m_pReader, &MPackReaderError );
    }

    void BinaryReader::EndReading()
    {
        EE_ASSERT( m_pReader != nullptr );
        mpack_reader_destroy( m_pReader );
        EE::Delete( m_pReader );
    }

    void BinaryReader::ReadValue( bool& v )
    {
        v = mpack_expect_bool( m_pReader );
    }

    void BinaryReader::ReadValue( int8_t& v )
    {
        v = mpack_expect_i8( m_pReader );
    }

    void BinaryReader::ReadValue( int16_t& v )
    {
        v = mpack_expect_i16( m_pReader );
    }

    void BinaryReader::ReadValue( int32_t& v )
    {
        v = mpack_expect_i32( m_pReader );
    }

    void BinaryReader::ReadValue( int64_t& v )
    {
        v = mpack_expect_i64( m_pReader );
    }

    void BinaryReader::ReadValue( uint8_t& v )
    {
        v = mpack_expect_u8( m_pReader );
    }

    void BinaryReader::ReadValue( uint16_t& v )
    {
        v = mpack_expect_u16( m_pReader );
    }

    void BinaryReader::ReadValue( uint32_t& v )
    {
        v = mpack_expect_u32( m_pReader );
    }

    void BinaryReader::ReadValue( uint64_t& v )
    {
        v = mpack_expect_u64( m_pReader );
    }

    void BinaryReader::ReadValue( float& v )
    {
        v = mpack_expect_float( m_pReader );
    }

    void BinaryReader::ReadValue( double& v )
    {
        v = mpack_expect_double( m_pReader );
    }

    void BinaryReader::ReadValue( Blob& blob )
    {
        size_t const expectedSize = mpack_expect_bin( m_pReader );
        blob.resize( expectedSize );

        mpack_read_bytes( m_pReader, (char*) blob.data(), expectedSize );
        mpack_done_bin( m_pReader );
    }

    void BinaryReader::ReadValue( String& v )
    {
        if ( mpack_peek_tag( m_pReader ).type == mpack_type_nil )
        {
            mpack_expect_nil( m_pReader );
            v.clear();
        }
        else
        {
            size_t const expectedLength = mpack_expect_str( m_pReader );
            v.resize( expectedLength + 1 );
            v.back() = 0;

            mpack_read_bytes( m_pReader, v.data(), expectedLength );
            mpack_done_str( m_pReader );
        }
    }

    void BinaryReader::ReadValue( StringID& v )
    {
        if ( mpack_peek_tag( m_pReader ).type == mpack_type_nil )
        {
            mpack_expect_nil( m_pReader );
            v = StringID();
        }
        else
        {
            InlineString str;

            size_t const expectedLength = mpack_expect_str( m_pReader );
            str.resize( expectedLength + 1 );
            str.back() = 0;

            mpack_read_bytes( m_pReader, str.data(), expectedLength );
            mpack_done_str( m_pReader );

            v = StringID( str.c_str() );
        }
    }

    void BinaryReader::ReadBinaryData( void* pData, size_t size )
    {
        size_t const expectedSize = mpack_expect_bin( m_pReader );
        EE_ASSERT( expectedSize == size );
        mpack_read_bytes( m_pReader, (char*) pData, expectedSize );
        mpack_done_bin( m_pReader );
    }

    //-------------------------------------------------------------------------

    static void MPackWriterError( mpack_writer_t* pWriter, mpack_error_t error )
    {
        EE_HALT();
    };

    BinaryWriter::~BinaryWriter()
    {
        Reset();
        EE_ASSERT( m_pData == nullptr );
    }

    void BinaryWriter::Reset()
    {
        if ( m_pWriter != nullptr )
        {
            EndWriting();
        }

        MPACK_FREE( m_pData );
        m_pData = nullptr;
        m_dataSize = 0;
    }

    void BinaryWriter::BeginWriting()
    {
        EE_ASSERT( m_pWriter == nullptr );
        m_pWriter = EE::New<mpack_writer_t>();
        mpack_writer_init_growable( m_pWriter, &m_pData, &m_dataSize );
        mpack_writer_set_error_handler( m_pWriter, &MPackWriterError );
    }

    void BinaryWriter::EndWriting()
    {
        EE_ASSERT( m_pWriter != nullptr );
        mpack_writer_destroy( m_pWriter );
        EE::Delete( m_pWriter );
    }

    void BinaryWriter::WriteValue( bool v )
    {
        mpack_write_bool( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( int8_t v )
    {
        mpack_write_i8( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( int16_t v )
    {
        mpack_write_i16( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( int32_t v )
    {
        mpack_write_i32( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( int64_t v )
    {
        mpack_write_i64( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( uint8_t v )
    {
        mpack_write_u8( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( uint16_t v )
    {
        mpack_write_u16( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( uint32_t v )
    {
        mpack_write_u32( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( uint64_t v )
    {
        mpack_write_u64( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( float v )
    {
        mpack_write_float( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( double v )
    {
        mpack_write_double( m_pWriter, v );
    }

    void BinaryWriter::WriteValue( Blob const& blob )
    {
        EE_ASSERT( !blob.empty() );
        mpack_write_bin( m_pWriter, (char*) blob.data(), (uint32_t) blob.size() );
    }

    void BinaryWriter::WriteValue( String const& v )
    {
        if ( v.empty() )
        {
            mpack_write_cstr_or_nil( m_pWriter, nullptr );
        }
        else
        {
            EE_ASSERT( v.c_str() != nullptr );
            mpack_write_cstr( m_pWriter, v.c_str() );
        }
    }

    void BinaryWriter::WriteValue( StringID const& v )
    {
        if ( v.IsValid() )
        {
            mpack_write_cstr_or_nil( m_pWriter, v.c_str() );
        }
        else
        {
            mpack_write_cstr_or_nil( m_pWriter, nullptr );
        }
    }

    void BinaryWriter::WriteBinaryData( void const* pData, size_t size )
    {
        EE_ASSERT( pData != nullptr && size != 0 );
        mpack_write_bin( m_pWriter, (char*) pData, (uint32_t) size );
    }

    //-------------------------------------------------------------------------

    BinaryInputArchive::~BinaryInputArchive()
    {
        Reset();
    }

    void BinaryInputArchive::Reset()
    {
        m_serializer.Reset();
        EE::Free( m_pFileData );
    }

    bool BinaryInputArchive::ReadFromData( uint8_t const* pData, size_t size )
    {
        if ( m_serializer.IsReading() )
        {
            m_serializer.Reset();
            EE::Free( m_pFileData );
        }

        m_serializer.BeginReading( (char const*) pData, size );
        return true;
    }

    bool BinaryInputArchive::ReadFromFile( FileSystem::Path const& filePath )
    {
        EE_ASSERT( filePath.IsFilePath() );

        if ( m_serializer.IsReading() )
        {
            m_serializer.Reset();
            EE::Free( m_pFileData );
        }

        //-------------------------------------------------------------------------

        if ( FileSystem::Exists( filePath ) )
        {
            FILE* pFile = fopen( filePath, "rb" );

            if ( pFile == nullptr )
            {
                return false;
            }

            fseek( pFile, 0, SEEK_END );
            m_fileDataSize = (size_t) ftell( pFile );
            fseek( pFile, 0, SEEK_SET );

            m_pFileData = EE::Alloc( m_fileDataSize );
            size_t const readLength = fread( m_pFileData, 1, m_fileDataSize, pFile );
            fclose( pFile );

            if ( readLength != m_fileDataSize )
            {
                return false;
            }

            m_serializer.BeginReading( (char*) m_pFileData, m_fileDataSize );

            return true;
        }

        return false;
    }

    bool BinaryInputArchive::ReadFromBlob( Blob const& blob )
    {
        return ReadFromData( blob.data(), blob.size() );
    }

    //-------------------------------------------------------------------------

    BinaryOutputArchive::BinaryOutputArchive()
    {
        m_serializer.BeginWriting();
    }

    void BinaryOutputArchive::Reset()
    {
        m_serializer.Reset();
        m_serializer.BeginWriting();
    }

    bool BinaryOutputArchive::WriteToFile( FileSystem::Path const& outPath )
    {
        if ( m_serializer.IsWriting() )
        {
            m_serializer.EndWriting();
        }

        if ( !outPath.EnsureDirectoryExists() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        FILE* pFile = fopen( outPath, "wb" );
        if ( pFile == nullptr )
        {
            return false;
        }

        fwrite( m_serializer.GetData(), m_serializer.GetSize(), 1, pFile );
        fclose( pFile );
        return true;
    }

    uint8_t* BinaryOutputArchive::GetBinaryData()
    {
        if ( m_serializer.IsWriting() )
        {
            m_serializer.EndWriting();
        }

        // This is UB but this data is a byte stream so whether it's char or unsigned char is irrelevant
        return (uint8_t*) m_serializer.GetData();
    }

    size_t BinaryOutputArchive::GetBinaryDataSize()
    {
        if ( m_serializer.IsWriting() )
        {
            m_serializer.EndWriting();
        }

        return m_serializer.GetSize();
    }

    void BinaryOutputArchive::GetAsBinaryBlob( Blob& outBlob )
    {
        if ( m_serializer.IsWriting() )
        {
            m_serializer.EndWriting();
        }

        outBlob.resize( m_serializer.GetSize() );
        memcpy( outBlob.data(), m_serializer.GetData(), m_serializer.GetSize() );
    }
}