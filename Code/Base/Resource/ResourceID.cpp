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

        // Only fourCC extensions allowed
        if( lastCharIdx - resourceExtensionStartIdx > 4 )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void ResourceID::OnPathChanged()
    {
        // Try to set the typeID
        if ( m_path.IsValid() )
        {
            auto const pExtension = m_path.GetExtension();
            if ( pExtension != nullptr )
            {
                m_type = ResourceTypeID( pExtension );
                return;
            }
        }

        // Invalidate this resource ID
        m_type = ResourceTypeID();
    }

    //-------------------------------------------------------------------------

    ResourceID ResourceID::GetParentResourceID() const
    {
        ResourceID parentResourceID;

        DataPath const parentDataFilePath = m_path.GetIntraFilePathParent();
        if ( parentDataFilePath.IsValid() )
        {
            parentResourceID = parentDataFilePath;
        }
        else
        {
            parentResourceID = *this;
        }

        return parentResourceID;
    }

    ResourceTypeID ResourceID::GetParentResourceTypeID() const
    {
        return GetParentResourceID().GetResourceTypeID();
    }
}