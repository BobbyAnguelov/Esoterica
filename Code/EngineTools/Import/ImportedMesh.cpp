#include "ImportedMesh.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    bool Mesh::IsValid() const
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

        if ( m_geometries.empty() )
        {
            return false;
        }

        return true;
    }

    void Mesh::ApplyScale( Float3 const& scale )
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

        for ( Geometry& geo : m_geometries )
        {
            for ( VertexData& vertex : geo.m_vertices )
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
            for ( Geometry& geo : m_geometries )
            {
                int32_t const numIndices = (int32_t) geo.m_indices.size();
                EE_ASSERT( ( numIndices % 3 ) == 0 );

                // Flip each triangle's winding
                for ( int32_t i = 0; i < numIndices; i += 3 )
                {
                    uint32_t originalI2 = geo.m_indices[i + 2];
                    geo.m_indices[i + 2] = geo.m_indices[i];
                    geo.m_indices[i] = originalI2;
                }
            }
        }
    }

    void Mesh::Finalize()
    {
        if ( IsSkeletalMesh() )
        {
            m_maxNumberOfBoneInfluences = 4;

            for ( auto const& geo : m_geometries )
            {
                m_maxNumberOfBoneInfluences = Math::Max( m_maxNumberOfBoneInfluences, geo.m_numBoneInfluences );
            }
        }
    }
}