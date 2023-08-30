#include "RawMesh.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    bool RawMesh::VertexData::operator==( VertexData const& rhs ) const
    {
        if ( !Vector( m_position ).IsNearEqual3( rhs.m_position, Vector::LargeEpsilon ) )
        {
            return false;
        }

        if ( !Vector( m_normal ).IsNearEqual3( rhs.m_normal, Vector::LargeEpsilon ) )
        {
            return false;
        }

        if ( !Vector( m_tangent ).IsNearEqual3( rhs.m_tangent, Vector::LargeEpsilon ) )
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

    String RawMesh::GetUniqueGeometrySectionName( String const& desiredName ) const
    {
        String finalName = desiredName;

        uint32_t counter = 0;
        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            isUniqueName = true;

            for ( auto i = 0; i < m_geometrySections.size(); i++ )
            {
                if ( finalName.comparei( m_geometrySections[i].m_name.c_str() ) == 0 )
                {
                    isUniqueName = false;
                    break;
                }
            }

            if ( !isUniqueName )
            {
                finalName.sprintf( "%s %u", desiredName.c_str(), counter );
                counter++;
            }
        }

        return finalName;
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

        for ( GeometrySection& GS : m_geometrySections )
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

    void RawMesh::MergeGeometrySectionsByMaterial()
    {
        struct MergeSet
        {
            bool operator==( StringID const& ID ) const { return m_ID == ID; }
            bool operator!=( StringID const& ID ) const { return m_ID != ID; }

            StringID                    m_ID;
            TInlineVector<int32_t,5>    m_sectionIndices;
        };

        TInlineVector<MergeSet, 10> mergeSets;
        bool requiresMerging = false;

        for ( auto i = 0; i < m_geometrySections.size(); i++ )
        {
            // Search merge sets for this ID
            int32_t mergeSetIdx = InvalidIndex;
            for( auto j = 0; j < mergeSets.size(); j++ )
            {
                if ( mergeSets[j].m_ID == m_geometrySections[i].m_materialNameID )
                {
                    mergeSetIdx = j;
                    break;
                }
            }

            // Fill merge sets
            if ( mergeSetIdx == InvalidIndex )
            {
                auto& mergeSet = mergeSets.emplace_back();
                mergeSet.m_ID = m_geometrySections[i].m_materialNameID;
                mergeSet.m_sectionIndices.emplace_back( i );
            }
            else
            {
                mergeSets[mergeSetIdx].m_sectionIndices.emplace_back( i );
                requiresMerging = true;
            }
        }

        //-------------------------------------------------------------------------

        if ( !requiresMerging )
        {
            return;
        }

        //-------------------------------------------------------------------------

        TVector<GeometrySection> mergedSections;

        for( auto& mergeSet : mergeSets )
        {
            auto& newSection = mergedSections.emplace_back();
            newSection.m_name = mergeSet.m_ID.c_str();
            newSection.m_materialNameID = mergeSet.m_ID;
            newSection.m_clockwiseWinding = m_geometrySections[mergeSet.m_sectionIndices.front()].m_clockwiseWinding;
            newSection.m_numUVChannels = m_geometrySections[mergeSet.m_sectionIndices.front()].m_numUVChannels;

            for ( int32_t sectionToMergeIdx : mergeSet.m_sectionIndices )
            {
                auto const& originalSection = m_geometrySections[sectionToMergeIdx];
                uint32_t indexOffset = (uint32_t) newSection.m_vertices.size();
                newSection.m_vertices.insert( newSection.m_vertices.end(), originalSection.m_vertices.begin(), originalSection.m_vertices.end() );

                for ( uint32_t idx : originalSection.m_indices )
                {
                    newSection.m_indices.emplace_back( indexOffset + idx );
                }
            }
        }

        m_geometrySections.swap( mergedSections );
    }
}