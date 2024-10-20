#pragma once

#include "ResourceTypeID.h"
#include "Base/FileSystem/DataPath.h"
#include "Base/Types/UUID.h"

//-------------------------------------------------------------------------
// Unique ID for a resource - Used for resource look up and dependencies
//-------------------------------------------------------------------------
// Resource ID's are the same as data paths except we extract the resource type from the extension
// Resource Type ID are generated from the lowercase extension of the resource path
//
// e.g. data://directory/someResource.mesh ->  ResourceTypeID = 'mesh'
// 
// We support the concept of sub-resource paths i.e. data://parentResource.pr/childResource.ch
// This allows us to specify resources that dont have source descriptor files, for example: navmeshes, graph variations, etc...
// When converting to file system paths, the sub-resources will be converted as follows:
//  <path to parent> + <parent resource name with extension> + <data path delimiter> + <sub-resource name with extension> -> <path to parent> + <parent resource name without extension> + <sub-resource delimiter> + <sub-resource name with extension>
//  e.g., data://parentResource.pr/childResource.ch -> c:\data\parentResource_childResource.ch
//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API ResourceID
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

    public:

        ResourceID() = default;
        ResourceID( DataPath const& path ) : m_path( path ) { EE_ASSERT( m_path.IsValid() ); OnPathChanged(); }
        ResourceID( DataPath&& path ) : m_path( std::move( path ) ) { EE_ASSERT( m_path.IsValid() ); OnPathChanged(); }
        inline ResourceID( String&& path ) : m_path( path ) { OnPathChanged(); }
        inline ResourceID( String const& path ) : m_path( path ) { OnPathChanged(); }
        inline ResourceID( char const* pPath ) : m_path( pPath ) { OnPathChanged(); }

        inline bool IsValid() const { return m_path.IsValid() && m_type.IsValid(); }
        inline void Clear() { m_path.Clear(); m_type.Clear(); }

        // Resource Info
        //-------------------------------------------------------------------------

        inline ResourceTypeID GetResourceTypeID() const { return m_type; }

        // Path Info
        //-------------------------------------------------------------------------

        inline bool IsPathSet() const { return m_path.IsValid(); }
        inline uint32_t GetPathID() const { return m_path.GetID(); }
        inline DataPath const& GetResourcePath() const { return m_path; }
        inline String GetFilenameWithoutExtension() const { EE_ASSERT( m_path.IsValid() ); return m_path.GetFilenameWithoutExtension(); }

        // Sub-resources
        //-------------------------------------------------------------------------

        inline bool IsSubResourceID() const { return m_path.IsIntraFilePath(); }

        // Get the parent resource ID for a sub-resource ID - if this is not a sub resource ID this function will return the same resource ID
        inline ResourceID GetParentResourceID() const;

        // Get the resource type ID for the parent of a sub-resource ID - if this is not a sub resource ID this function will return the same resource type ID
        ResourceTypeID GetParentResourceTypeID() const;

        // Conversion
        //-------------------------------------------------------------------------

        inline String const& ToString() const { return m_path.GetString(); }
        inline char const* c_str() const { return m_path.c_str(); }
        inline FileSystem::Path GetFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const { return m_path.GetFileSystemPath( dataDirectoryPath ); }
        inline FileSystem::Path GetParentResourceFileSystemPath( FileSystem::Path const& dataDirectoryPath ) const { return m_path.GetParentFileSystemPath( dataDirectoryPath ); }

        // Operators
        //-------------------------------------------------------------------------

        inline bool operator==( ResourceID const& rhs ) const { return m_path == rhs.m_path; }
        inline bool operator!=( ResourceID const& rhs ) const { return m_path != rhs.m_path; }

        inline bool operator==( uint32_t const& ID ) const { return m_path.GetID() == ID; }
        inline bool operator!=( uint32_t const& ID ) const { return m_path.GetID() != ID; }

    private:

        void OnPathChanged();

    private:

        DataPath            m_path;
        ResourceTypeID      m_type;
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