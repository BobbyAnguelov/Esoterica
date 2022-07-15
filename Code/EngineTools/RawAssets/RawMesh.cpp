#include "RawMesh.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    bool RawMesh::VertexData::operator==( VertexData const& rhs ) const
    {
        if ( m_position != rhs.m_position )
        {
            return false;
        }

        if ( m_normal != rhs.m_normal )
        {
            return false;
        }

        if ( m_tangent != rhs.m_tangent )
        {
            return false;
        }

        if ( m_texCoords.size() != rhs.m_texCoords.size() )
        {
            return false;
        }

        int32_t const numTexCoords = (int32_t) m_texCoords.size();
        for ( int32_t i = 0; i < numTexCoords; i++ )
        {
            if ( m_texCoords[i] != rhs.m_texCoords[i] )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool RawMesh::IsValid() const
    {
        if ( HasErrors() )
        {
            return false;
        }

        if ( m_isSkeletalMesh )
        {
            if ( !m_skeleton.IsValid() )
            {
                return false;
            }
        }

        if ( m_geometrySections.empty() )
        {
            return false;
        }

        return true;
    }
}