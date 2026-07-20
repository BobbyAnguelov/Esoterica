#include "ResourceID.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool ResourceID::IsValidResourceIDString( char const* pStr )
    {
        EE_ASSERT( pStr != nullptr );

        size_t const length = strlen( pStr );
        if ( length == 0 )
        {
            return false;
        }

        // Ensure this is a valid data path
        if ( !DataPath::IsValidPath( pStr ) )
        {
            return false;
        }

        // Check that we have a resource ID extension
        char const* pFoundExtensionStart = strrchr( pStr, '.' );
        if ( pFoundExtensionStart == nullptr )
        {
            return false;
        }

        size_t const lastCharIdx = length - 1;
        size_t const resourceExtensionStartIdx = pFoundExtensionStart - pStr;

        // Only eightCC extensions allowed
        if( lastCharIdx - resourceExtensionStartIdx > 8 )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void ResourceID::OnPathChanged()
    {
        m_type = ResourceTypeID();
        if ( m_path.IsValid() )
        {
            FileSystem::Extension const extension = m_path.HasSubFilename() ? m_path.GetSubFilenameExtension() : m_path.GetExtension();
            if ( !extension.empty() && ResourceTypeID::IsValidResourceTypeIdentifierString( extension ) )
            {
                m_type = ResourceTypeID( extension );
            }
        }
    }

    FileSystem::Path ResourceID::GetCompiledFileSystemPath( FileSystem::Path const& compiledDataDirectoryPath ) const
    {
        if ( IsSubResourceID() )
        {
            return m_path.GetFlattenedFileSystemPath( compiledDataDirectoryPath );
        }
        else
        {
            return m_path.GetFileSystemPath( compiledDataDirectoryPath );
        }
    }

    //-------------------------------------------------------------------------

    ResourceID ResourceID::GetParentResourceID() const
    {
        EE_ASSERT( IsValid() && IsSubResourceID() );
        return ResourceID( m_path.GetPathWithoutSubFilename() );
    }

    ResourceTypeID ResourceID::GetParentResourceTypeID() const
    {
        EE_ASSERT( IsValid() && IsSubResourceID() );
        return ResourceTypeID( m_path.GetExtension() );
    }

    String ResourceID::GetSubResourceName() const
    {
        EE_ASSERT( IsValid() && IsSubResourceID() );
        return m_path.GetSubFilename();
    }

    String ResourceID::GetSubResourceNameWithoutExtension() const
    {
        EE_ASSERT( IsValid() && IsSubResourceID() );
        return m_path.GetSubFilenameWithoutExtension();
    }

    void ResourceID::SetSubResourceName( char const* pResourceName )
    {
        m_path.SetSubFilename( pResourceName );
        OnPathChanged();
    }
}