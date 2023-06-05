#pragma once

#include "ResourceTypeID.h"
#include "ResourcePath.h"
#include "System/Types/UUID.h"

//-------------------------------------------------------------------------
// Unique ID for a resource - Used for resource look up and dependencies
//-------------------------------------------------------------------------
// Resource ID's are the same as data paths except we extract the resource type from the extension
// Resource Type ID are generated from the lowercase extension of the resource path
//
// e.g. data://directory/someResource.mesh ->  ResourceTypeID = 'mesh'
//-------------------------------------------------------------------------

namespace EE
{
    class EE_SYSTEM_API ResourceID
    {
        EE_CUSTOM_SERIALIZE_WRITE_FUNCTION( archive )
        {
            archive << m_path;
            return archive;
        }

        EE_CUSTOM_SERIALIZE_READ_FUNCTION( archive )
        {
            archive << m_path;
            OnPathChanged();
            return archive;
        }

    public:

        static bool IsValidResourceIDString( char const* pStr );
        inline static bool IsValidResourceIDString( String const& str ) { return IsValidResourceIDString( str.c_str() ); }
        inline static ResourceID FromFileSystemPath( FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& filePath ) { return ResourceID( ResourcePath::FromFileSystemPath( rawResourceDirectoryPath, filePath ) ); }

    public:

        ResourceID() = default;
        ResourceID( ResourcePath const& path ) : m_path( path ) { EE_ASSERT( m_path.IsValid() ); OnPathChanged(); }
        ResourceID( ResourcePath&& path ) : m_path( std::move( path ) ) { EE_ASSERT( m_path.IsValid() ); OnPathChanged(); }
        inline ResourceID( String&& path ) : m_path( path ) { OnPathChanged(); }
        inline ResourceID( String const& path ) : m_path( path ) { OnPathChanged(); }
        inline ResourceID( char const* pPath ) : m_path( pPath ) { OnPathChanged(); }

        inline bool IsValid() const { return m_type.IsValid(); }
        inline uint32_t GetPathID() const { return m_path.GetID(); }

        inline ResourcePath const& GetResourcePath() const { return m_path; }
        inline ResourceTypeID GetResourceTypeID() const { return m_type; }
        inline String GetFileNameWithoutExtension() const { EE_ASSERT( m_path.IsValid() ); return m_path.GetFileNameWithoutExtension(); }

        inline void Clear() { m_path.Clear(); m_type.Clear(); }

        //-------------------------------------------------------------------------

        inline String const& ToString() const { return m_path.GetString(); }
        inline FileSystem::Path ToFileSystemPath( FileSystem::Path const& rawResourceDirectoryPath ) const { return m_path.ToFileSystemPath( rawResourceDirectoryPath ); }
        inline char const* c_str() const { return m_path.c_str(); }

        //-------------------------------------------------------------------------

        inline bool operator==( ResourceID const& rhs ) const { return m_path == rhs.m_path; }
        inline bool operator!=( ResourceID const& rhs ) const { return m_path != rhs.m_path; }

        inline bool operator==( uint32_t const& ID ) const { return m_path.GetID() == ID; }
        inline bool operator!=( uint32_t const& ID ) const { return m_path.GetID() != ID; }

    private:

        void OnPathChanged();

    private:

        ResourcePath            m_path;
        ResourceTypeID          m_type;
    };
}

//-------------------------------------------------------------------------
// Support for THashMap

namespace eastl
{
    template <>
    struct hash<EE::ResourceID>
    {
        eastl_size_t operator()( EE::ResourceID const& ID ) const
        {
            return ( uint32_t ) ID.GetPathID();
        }
    };
}