#include "ImportedSkeleton.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    ImportedSkeleton::Bone::Bone( char const* pName )
        : m_name( pName )
    {
        EE_ASSERT( m_name.IsValid() );
    }

    //-------------------------------------------------------------------------

    int32_t ImportedSkeleton::GetBoneIndex( StringID const& boneName ) const
    {
        int32_t const numBones = GetNumBones();

        for ( auto i = 0; i < numBones; i++ )
        {
            if ( m_bones[i].m_name == boneName )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    void ImportedSkeleton::CalculateLocalTransforms()
    {
        EE_ASSERT( !m_bones.empty() );
        m_bones[0].m_localTransform = m_bones[0].m_globalTransform;

        auto const numBones = m_bones.size();
        for ( auto i = numBones - 1; i > 0; i-- )
        {
            int32_t parentIdx = m_bones[i].m_parentBoneIdx;
            m_bones[i].m_localTransform = Transform::Delta( m_bones[parentIdx].m_globalTransform, m_bones[i].m_globalTransform );
        }
    }

    void ImportedSkeleton::CalculateGlobalTransforms()
    {
        EE_ASSERT( !m_bones.empty() );
        m_bones[0].m_globalTransform = m_bones[0].m_localTransform;

        auto const numBones = m_bones.size();
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t parentIdx = m_bones[i].m_parentBoneIdx;
            EE_ASSERT( parentIdx != InvalidIndex );
            m_bones[i].m_globalTransform = m_bones[i].m_localTransform * m_bones[parentIdx].m_globalTransform;
        }
    }

    bool ImportedSkeleton::IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const
    {
        EE_ASSERT( parentBoneIdx >= 0 && parentBoneIdx < m_bones.size() );
        EE_ASSERT( childBoneIdx >= 0 && childBoneIdx < m_bones.size() );

        bool isChild = false;

        int32_t actualParentBoneIdx = GetParentBoneIndex( childBoneIdx );
        while ( actualParentBoneIdx != InvalidIndex )
        {
            if ( actualParentBoneIdx == parentBoneIdx )
            {
                isChild = true;
                break;
            }

            actualParentBoneIdx = GetParentBoneIndex( actualParentBoneIdx );
        }

        return isChild;
    }

    void ImportedSkeleton::Finalize( TVector<StringID> const& highLODBones )
    {
        if ( highLODBones.empty() )
        {
            m_numBonesToSampleAtLowLOD = GetNumBones();
        }
        else
        {
            TVector<StringID> finalSetOfHighLODBones = highLODBones;
            TInlineVector<int32_t, 100> highLODBoneIndices;

            // Validate LOD setup
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < (int32_t) finalSetOfHighLODBones.size(); i++ )
            {
                if ( !finalSetOfHighLODBones[i].IsValid() )
                {
                    LogWarning( "Invalid boneID in high LOD settings at index %d", i );
                    finalSetOfHighLODBones.erase( finalSetOfHighLODBones.begin() + i );
                    i--;
                }
                else
                {
                    int32_t const highLODBoneIdx = GetBoneIndex( finalSetOfHighLODBones[i] );
                    if ( highLODBoneIdx == InvalidIndex )
                    {
                        LogWarning( "Unknown boneID in high LOD settings: %s", finalSetOfHighLODBones[i].c_str() );
                        finalSetOfHighLODBones.erase( finalSetOfHighLODBones.begin() + i );
                        i--;
                    }
                    else // Add to index list and ensure all children of this bone are in the high LOD list (we dont allow high LOD bones to be sandwiched by low LOD bones)
                    {
                        highLODBoneIndices.emplace_back( highLODBoneIdx );

                        // Children are always listed after their parents
                        for ( uint32_t childBoneIdx = i + 1; childBoneIdx < GetNumBones(); childBoneIdx++ )
                        {
                            if ( IsChildBoneOf( highLODBoneIdx, childBoneIdx ) )
                            {
                                StringID const childBoneID = GetBoneID( childBoneIdx );
                                if ( !VectorContains( finalSetOfHighLODBones, childBoneID ) )
                                {
                                    finalSetOfHighLODBones.emplace_back( childBoneID );
                                }
                            }
                        }
                    }
                }
            }

            // Sort high LOD bones by index
            eastl::sort( highLODBoneIndices.begin(), highLODBoneIndices.end() );

            //-------------------------------------------------------------------------

            TVector<Bone> reorderedBoneData;

            // Add low LOD bones first
            int32_t const numBones = GetNumBones();
            for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                if ( VectorContains( highLODBoneIndices, boneIdx ) )
                {
                    continue;
                }

                reorderedBoneData.emplace_back( GetBoneData( boneIdx ) );
            }

            m_numBonesToSampleAtLowLOD = numBones - (int32_t) highLODBoneIndices.size();

            // Add high LOD bones
            for ( auto boneIdx : highLODBoneIndices )
            {
                reorderedBoneData.emplace_back( GetBoneData( boneIdx ) );
            }

            // Swap bone data and update all parent bone indices
            //-------------------------------------------------------------------------

            m_bones.swap( reorderedBoneData );

            for ( auto& boneData : m_bones )
            {
                boneData.m_parentBoneIdx = GetBoneIndex( boneData.m_parentBoneName );
            }
        }
    }
}