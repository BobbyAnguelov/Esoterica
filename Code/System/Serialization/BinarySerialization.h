#pragma once

#include "System/Types/Containers_ForwardDecl.h"
#include <type_traits>

//-------------------------------------------------------------------------

struct mpack_reader_t;
struct mpack_writer_t;

namespace EE { class StringID; }
namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

namespace EE::Serialization
{
    //-------------------------------------------------------------------------
    // Version
    //-------------------------------------------------------------------------
    // Update this value to cause all resources to recompile

    EE_SYSTEM_API int32_t GetBinarySerializationVersion();

    //-------------------------------------------------------------------------
    // Binary Reader/Writer
    //-------------------------------------------------------------------------
    // These classes actually perform the serialization of the data
    // We've split them out so that we can reuse the same serializer shell and so we can keep all the impl in the cpp

    class EE_SYSTEM_API BinaryReader final
    {
    public:

        BinaryReader() = default;
        ~BinaryReader();

        BinaryReader( BinaryReader const& rhs ) = delete;
        BinaryReader& operator=( BinaryReader const& rhs ) = delete;
        BinaryReader( BinaryReader&& rhs ) { m_pReader = rhs.m_pReader; rhs.m_pReader = nullptr; }
        BinaryReader& operator=( BinaryReader&& rhs ) { m_pReader = rhs.m_pReader; rhs.m_pReader = nullptr; return *this; }

        void Reset();

        inline bool IsReading() const { return m_pReader != nullptr; }
        void BeginReading( char const* pData, size_t size );
        void EndReading();

        void ReadValue( bool& v );
        void ReadValue( int8_t& v );
        void ReadValue( int16_t& v );
        void ReadValue( int32_t& v );
        void ReadValue( int64_t& v );
        void ReadValue( uint8_t& v );
        void ReadValue( uint16_t& v );
        void ReadValue( uint32_t& v );
        void ReadValue( uint64_t& v );
        void ReadValue( float& v );
        void ReadValue( double& v );
        void ReadValue( Blob& blob );
        void ReadValue( String& v );
        void ReadValue( StringID& v);

        void ReadBinaryData( void* pData, size_t size );

    private:

        mpack_reader_t* m_pReader = nullptr;
    };

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API BinaryWriter final
    {
    public:

        BinaryWriter() = default;
        ~BinaryWriter();

        BinaryWriter( BinaryWriter const& rhs ) = delete;
        BinaryWriter& operator=( BinaryWriter const& rhs ) = delete;
        BinaryWriter( BinaryWriter&& rhs ) { m_pWriter = rhs.m_pWriter; rhs.m_pWriter = nullptr; }
        BinaryWriter& operator=( BinaryWriter&& rhs ) { m_pWriter = rhs.m_pWriter; rhs.m_pWriter = nullptr; return *this; }

        void Reset();

        inline bool IsWriting() const { return m_pWriter != nullptr; }
        void BeginWriting();
        void EndWriting();

        inline char* GetData() const { return m_pData; }
        inline size_t GetSize() const { return m_dataSize; }

        void WriteValue( bool v );
        void WriteValue( int8_t v );
        void WriteValue( int16_t v );
        void WriteValue( int32_t v );
        void WriteValue( int64_t v );
        void WriteValue( uint8_t v );
        void WriteValue( uint16_t v );
        void WriteValue( uint32_t v );
        void WriteValue( uint64_t v );
        void WriteValue( float v );
        void WriteValue( double v );
        void WriteValue( Blob const& blob );
        void WriteValue( String const& v );
        void WriteValue( StringID const& v );

        void WriteBinaryData( void const* pData, size_t size );

    private:

        mpack_writer_t*     m_pWriter = nullptr;
        char*               m_pData = nullptr;
        size_t              m_dataSize;
    };

    //-------------------------------------------------------------------------
    // Serialization Archive
    //-------------------------------------------------------------------------
    // Primary API for binary serialization.

    namespace Internal
    {
        // Helper to explicitly flag base class serialization
        template<typename Base>
        struct SerializeBaseType
        {
            template<typename Derived> SerializeBaseType( Derived* pDerived ) : m_instance( *reinterpret_cast<Base*>( pDerived ) ) {}
            template<typename Derived> SerializeBaseType( Derived const* pDerived ) : m_instance( const_cast<Base&>( *reinterpret_cast<Base const*>( pDerived ) ) ) {}
            Base& m_instance;
        };

        // Primary archive interface
        template<typename Serializer>
        class Archive
        {
        public:

            // Serialize type
            //-------------------------------------------------------------------------

            template<typename T>
            Archive& operator<<( T& value )
            {
                // Handle enums
                if constexpr ( std::is_enum<T>::value )
                {
                    if constexpr ( std::is_same<Serializer, BinaryReader>::value )
                    {
                        std::underlying_type_t<T> numericValue;
                        m_serializer.ReadValue( numericValue );
                        value = (T) numericValue;
                    }
                    else
                    {
                        auto numericValue = static_cast<std::underlying_type_t<T>>( value );
                        m_serializer.WriteValue( numericValue );
                    }
                }
                // If this is a structure and not a string, try to call the explicit serialize method on it
                else if constexpr ( std::is_class<T>::value && !std::is_same<T, EE::String>::value && !std::is_same<T, EE::StringID>::value )
                {
                    value.Serialize( *this );
                }
                else // Directly serialize POD types (and strings)
                {
                    if constexpr ( std::is_same<Serializer, BinaryReader>::value )
                    {
                        m_serializer.ReadValue( value );
                    }
                    else
                    {
                        m_serializer.WriteValue( value );
                    }
                }
                return *this;
            }

            template<typename T>
            Archive& operator<<( T const& value )
            {
                return operator<<( const_cast<T&>( value ) );
            }

            // Serialize base members
            //-------------------------------------------------------------------------

            template<typename T>
            Archive& operator<<( SerializeBaseType<T> baseTypeInstance )
            {
                baseTypeInstance.m_instance.Serialize( *this );
                return *this;
            }

            // Serialize fixed arrays
            //-------------------------------------------------------------------------

            template<typename T, size_t S>
            Archive& SerializeFixedArray( T* pArrayData )
            {
                uint64_t numElements = 0;

                // Serialize size
                if constexpr ( std::is_same<Serializer, BinaryReader>::value )
                {
                    m_serializer.ReadValue( numElements );
                    EE_ASSERT( numElements == S );
                }
                else
                {
                    numElements = S;
                    m_serializer.WriteValue( numElements );
                }

                SerializeArrayElements( pArrayData, numElements );

                return *this;
            }

            template<typename T, int S>
            Archive& operator<<( T( &arr )[S] )
            {
                return SerializeFixedArray<T, S>( arr );
            }

            template<typename T, int S>
            Archive& operator<<( T const ( &arr )[S] )
            {
                return SerializeFixedArray<T, S>( const_cast<T*>( arr ) );
            }

            // Serialize dynamic arrays
            //-------------------------------------------------------------------------

            template<typename T>
            Archive& operator<<( TVector<T>& arr )
            {
                uint64_t numElements = 0;

                // Serialize size
                if constexpr ( std::is_same<Serializer, BinaryReader>::value )
                {
                    m_serializer.ReadValue( numElements );
                    arr.resize( numElements );
                }
                else
                {
                    numElements = arr.size();
                    m_serializer.WriteValue( numElements );
                }

                SerializeArrayElements( arr.data(), numElements );

                return *this;
            }

            template<typename T>
            Archive& operator<<( TVector<T> const& arr )
            {
                return operator<<( const_cast<TVector<T>&>( arr ) );
            }

            // Serialize inline arrays
            //-------------------------------------------------------------------------

            template<typename T, size_t S>
            Archive& operator<<( TInlineVector<T, S>& arr )
            {
                uint64_t numElements = 0;

                // Serialize size
                if constexpr ( std::is_same<Serializer, BinaryReader>::value )
                {
                    m_serializer.ReadValue( numElements );
                    arr.resize( numElements );
                }
                else
                {
                    numElements = arr.size();
                    m_serializer.WriteValue( numElements );
                }

                SerializeArrayElements( arr.data(), numElements );
                return *this;
            }

            template<typename T, size_t S>
            Archive& operator<<( TInlineVector<T, S> const& arr )
            {
                return operator<<( const_cast<TInlineVector<T, S>&>( arr ) );
            }

            // Serialize hash maps
            //-------------------------------------------------------------------------

            template<typename K, typename V>
            Archive& operator<<( THashMap<K,V>& map )
            {
                if constexpr ( std::is_same<Serializer, BinaryReader>::value )
                {
                    uint32_t size = 0;
                    m_serializer.ReadValue( size );
                    map.reserve( size );

                    K key;
                    V value;

                    for ( auto i = 0u; i < size; i++ )
                    {
                        operator<<( key );
                        operator<<( value );
                        map.insert( TPair<K, V>( key, value ) );
                    }
                }
                else // Writing
                {
                    uint32_t const size = (uint32_t) map.size();
                    m_serializer.WriteValue( size );

                    for ( auto& pair : map )
                    {
                        K const& key = pair.first;
                        V const& value = pair.second;
                        operator<<( key );
                        operator<<( value );
                    }
                }

                return *this;
            }

            template<typename K, typename V>
            Archive& operator<<( THashMap<K, V> const& arr )
            {
                return operator<<( const_cast<THashMap<K, V>&>( arr ) );
            }

            // Serialize Blobs
            //-------------------------------------------------------------------------
            // This is needed so we dont serialize it as an array

            template<>
            Archive& operator<<( Blob& blob )
            {
                if constexpr ( std::is_same<Serializer, BinaryReader>::value )
                {
                    m_serializer.ReadValue( blob );
                }
                else // Writing
                {
                    m_serializer.WriteValue( blob );
                }

                return *this;
            }

            template<>
            Archive& operator<<( Blob const& blob )
            {
                return operator<<( const_cast<Blob&>( blob ) );
            }

            // Fold expression to allow for the serialize macros to work
            //-------------------------------------------------------------------------

            template<typename... Values>
            Archive& Serialize( Values&&... values )
            {
                ( ( *this ) << ... << values );
                return *this;
            }

        private:

            template<typename T>
            void SerializeArrayElements( T* pArrayData, size_t numElements )
            {
                // Nothing to serialize
                if ( numElements == 0 )
                {
                    return;
                }

                // If we are a basic type, then serialize as a block of binary data
                if constexpr ( std::is_integral<T>::value || std::is_floating_point<T>::value )
                {
                    // Read
                    if constexpr ( std::is_same<Serializer, BinaryReader>::value )
                    {
                        size_t const dataSize = sizeof( T ) * numElements;
                        m_serializer.ReadBinaryData( pArrayData, dataSize );
                    }
                    else // Write data
                    {
                        size_t const dataSize = sizeof( T ) * numElements;
                        m_serializer.WriteBinaryData( pArrayData, dataSize );
                    }
                }
                else // Individually serialize each element
                {
                    for ( auto i = 0u; i < numElements; i++ )
                    {
                        operator<<( pArrayData[i] );
                    }
                }
            }

        protected:

            Serializer m_serializer;
        };
    }

    //-------------------------------------------------------------------------
    // Archives
    //-------------------------------------------------------------------------

    class EE_SYSTEM_API BinaryInputArchive final : public Internal::Archive<BinaryReader>
    {
    public:

        ~BinaryInputArchive();

        // Clears all loaded data and reset the archives to an invalid state
        void Reset();

        bool ReadFromData( uint8_t const* pData, size_t size );
        bool ReadFromBlob( Blob const& blob );
        bool ReadFromFile( FileSystem::Path const& filePath );

    private:

        void*       m_pFileData = nullptr;
        size_t      m_fileDataSize = 0;
    };

    //-------------------------------------------------------------------------

    // Note:    The binary writer is modal (start/stop) and writing cannot be restarted once stopped.
    //          It is intended to be used in a single serialization pass
    class EE_SYSTEM_API BinaryOutputArchive final : public Internal::Archive<BinaryWriter>
    {
    public:

        BinaryOutputArchive();

        // Clears all written data and begins writing again
        void Reset();

        // Stops writing and outputs the binary data to a file
        bool WriteToFile( FileSystem::Path const& outPath );

        // Stops writing and gets all written binary data
        uint8_t* GetBinaryData();

        // Stops writing and gets the size of the written binary data
        size_t GetBinaryDataSize();

        // Gets the binary data as a blob
        void GetAsBinaryBlob( Blob& outBlob );
    };
}

//-------------------------------------------------------------------------

#define EE_SERIALIZE( ... )\
friend Serialization::Internal::Archive<Serialization::BinaryReader>;\
friend Serialization::Internal::Archive<Serialization::BinaryWriter>;\
Serialization::Internal::Archive<Serialization::BinaryReader>& Serialize( Serialization::Internal::Archive<Serialization::BinaryReader>& ar ) { ar.Serialize( __VA_ARGS__ ); return ar; }\
Serialization::Internal::Archive<Serialization::BinaryWriter>& Serialize( Serialization::Internal::Archive<Serialization::BinaryWriter>& ar ) { ar.Serialize( __VA_ARGS__ ); return ar; }

#define EE_SERIALIZE_BASE( BaseTypeName ) Serialization::Internal::SerializeBaseType<BaseTypeName>( this )

//-------------------------------------------------------------------------

#define EE_CUSTOM_SERIALIZE_READ_FUNCTION( archive )\
friend Serialization::Internal::Archive<Serialization::BinaryReader>;\
friend Serialization::Internal::Archive<Serialization::BinaryWriter>;\
Serialization::Internal::Archive<Serialization::BinaryReader>& Serialize( Serialization::Internal::Archive<Serialization::BinaryReader>& archive )

#define EE_CUSTOM_SERIALIZE_WRITE_FUNCTION( archive ) Serialization::Internal::Archive<Serialization::BinaryWriter>& Serialize( Serialization::Internal::Archive<Serialization::BinaryWriter>& archive )