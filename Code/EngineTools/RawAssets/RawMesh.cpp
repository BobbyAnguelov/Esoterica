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

    void RawMesh::ApplyScale( Float3 const& scale )
    {
        EE_ASSERT( !IsSkeletalMesh() );

        //-------------------------------------------------------------------------

        Vector const vScale( scale );
        Vector const vReciprocalScale = vScale.GetReciprocal();

        if ( vScale.IsNearEqual3( Vector::One ) )
        {
            return;
        }

        // Apply scaling
        //-------------------------------------------------------------------------

        Matrix scalingMatrix;
        scalingMatrix.m_rows[0] = _mm_and_ps( vScale, SIMD::g_maskX000 );
        scalingMatrix.m_rows[1] = _mm_and_ps( vScale, SIMD::g_mask0Y00 );
        scalingMatrix.m_rows[2] = _mm_and_ps( vScale, SIMD::g_mask00Z0 );
        scalingMatrix.m_rows[3] = Vector::UnitW;

        Matrix normalScalingMatrix;
        normalScalingMatrix.m_rows[0] = _mm_and_ps( vReciprocalScale, SIMD::g_maskX000 );
        normalScalingMatrix.m_rows[1] = _mm_and_ps( vReciprocalScale, SIMD::g_mask0Y00 );
        normalScalingMatrix.m_rows[2] = _mm_and_ps( vReciprocalScale, SIMD::g_mask00Z0 );
        normalScalingMatrix.m_rows[3] = Vector::UnitW;

        for( GeometrySection& GS : m_geometrySections )
        {
            for ( VertexData& vertex : GS.m_vertices )
            {
                vertex.m_position = scalingMatrix.TransformPoint( vertex.m_position );
                vertex.m_normal = normalScalingMatrix.TransformNormal( vertex.m_normal ).GetNormalized3();
                vertex.m_tangent = normalScalingMatrix.TransformPoint( vertex.m_tangent ).GetNormalized3();
                vertex.m_binormal = normalScalingMatrix.TransformPoint( vertex.m_binormal ).GetNormalized3();
            }
        }

        // Fix winding
        //-------------------------------------------------------------------------

        int32_t numNegativelyScaledAxes = ( scale.m_x < 0 ) ? 1 : 0;
        numNegativelyScaledAxes += ( scale.m_y < 0 ) ? 1 : 0;
        numNegativelyScaledAxes += ( scale.m_z < 0 ) ? 1 : 0;

        bool const flipWindingDueToScale = Math::IsOdd( numNegativelyScaledAxes );
        if ( flipWindingDueToScale )
        {
            for ( GeometrySection& GS : m_geometrySections )
            {
                int32_t const numIndices = (int32_t) GS.m_indices.size();
                EE_ASSERT( ( numIndices % 3 ) == 0 );

                // Flip each triangle's winding
                for ( int32_t i = 0; i < numIndices; i += 3 )
                {
                    uint32_t originalI2 = GS.m_indices[i + 2];
                    GS.m_indices[i + 2] = GS.m_indices[i];
                    GS.m_indices[i] = originalI2;
                }
            }
        }
    }
}