#include "AnimationPose.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    Pose::Pose( Skeleton const* pSkeleton, Type initialState )
        : m_pSkeleton( pSkeleton )
        , m_parentSpaceTransforms( pSkeleton->GetNumBones() )
    {
        EE_ASSERT( pSkeleton != nullptr );
        Reset( initialState );
    }

    Pose::Pose( Pose&& rhs )
    {
        EE_ASSERT( rhs.m_pSkeleton != nullptr );
        operator=( eastl::move( rhs ) );
    }

    Pose::Pose( Pose const& rhs )
    {
        EE_ASSERT( rhs.m_pSkeleton != nullptr );
        operator=( rhs );
    }

    Pose& Pose::operator=( Pose&& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_parentSpaceTransforms.swap( rhs.m_parentSpaceTransforms );
        m_modelSpaceTransforms.swap( rhs.m_modelSpaceTransforms );
        m_state = rhs.m_state;

        return *this;
    }

    Pose& Pose::operator=( Pose const& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_parentSpaceTransforms = rhs.m_parentSpaceTransforms;
        m_modelSpaceTransforms = rhs.m_modelSpaceTransforms;
        m_state = rhs.m_state;

        return *this;
    }

    void Pose::CopyFrom( Pose const& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_parentSpaceTransforms = rhs.m_parentSpaceTransforms;
        m_modelSpaceTransforms = rhs.m_modelSpaceTransforms;
        m_state = rhs.m_state;
    }

    void Pose::SwapWith( Pose& rhs )
    {
        Skeleton const* const pTempSkeleton = m_pSkeleton;
        m_pSkeleton = rhs.m_pSkeleton;
        rhs.m_pSkeleton = pTempSkeleton;

        State const tempState = m_state;
        m_state = rhs.m_state;
        rhs.m_state = tempState;

        m_parentSpaceTransforms.swap( rhs.m_parentSpaceTransforms );
        m_modelSpaceTransforms.swap( rhs.m_modelSpaceTransforms );
    }

    //-------------------------------------------------------------------------

    void Pose::Reset( Type initialState, bool calculateModelSpacePose )
    {
        switch ( initialState )
        {
            case Type::ReferencePose:
            {
                SetToReferencePose( calculateModelSpacePose );
            }
            break;

            case Type::ZeroPose:
            {
                SetToZeroPose( calculateModelSpacePose );
            }
            break;

            default:
            {
                // Leave memory intact, just change state
                m_state = State::Unset;
            }
            break;
        }
    }

    void Pose::SetToReferencePose( bool setGlobalPose )
    {
        m_parentSpaceTransforms = m_pSkeleton->GetLocalReferencePose();

        if ( setGlobalPose )
        {
            m_modelSpaceTransforms = m_pSkeleton->GetGlobalReferencePose();
        }
        else
        {
            m_modelSpaceTransforms.clear();
        }

        m_state = State::ReferencePose;
    }

    void Pose::SetToZeroPose( bool setGlobalPose )
    {
        auto const numBones = m_pSkeleton->GetNumBones();
        m_parentSpaceTransforms.clear();
        m_parentSpaceTransforms.resize( numBones, Transform::Identity );

        if ( setGlobalPose )
        {
            m_modelSpaceTransforms = m_parentSpaceTransforms;
        }
        else
        {
            m_modelSpaceTransforms.clear();
        }

        m_state = State::ZeroPose;
    }

    //-------------------------------------------------------------------------

    void Pose::CalculateModelSpaceTransforms( Skeleton::LOD lod )
    {
        int32_t const numTotalBones = m_pSkeleton->GetNumBones( Skeleton::LOD::High );
        int32_t const numRelevantBones = m_pSkeleton->GetNumBones( lod );
        m_modelSpaceTransforms.resize( numTotalBones );

        m_modelSpaceTransforms[0] = m_parentSpaceTransforms[0];
        for ( int32_t boneIdx = 1; boneIdx < numRelevantBones; boneIdx++ )
        {
            int32_t const parentIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
            EE_ASSERT( parentIdx < boneIdx );
            m_modelSpaceTransforms[boneIdx] = m_parentSpaceTransforms[boneIdx] * m_modelSpaceTransforms[parentIdx];
        }
    }

    Transform Pose::GetGlobalTransform( int32_t boneIdx ) const
    {
        EE_ASSERT( boneIdx < m_pSkeleton->GetNumBones() );

        Transform boneGlobalTransform;
        if ( !m_modelSpaceTransforms.empty() )
        {
            boneGlobalTransform = m_modelSpaceTransforms[boneIdx];
        }
        else
        {
            auto boneParents = EE_STACK_ARRAY_ALLOC( int32_t, m_pSkeleton->GetNumBones() );
            int32_t nextEntry = 0;

            // Get parent list
            int32_t parentIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
            while ( parentIdx != InvalidIndex )
            {
                boneParents[nextEntry++] = parentIdx;
                parentIdx = m_pSkeleton->GetParentBoneIndex( parentIdx );
            }

            // If we have parents
            boneGlobalTransform = m_parentSpaceTransforms[boneIdx];
            if ( nextEntry > 0 )
            {
                // Calculate global transform of parent
                int32_t arrayIdx = nextEntry - 1;
                parentIdx = boneParents[arrayIdx--];
                auto parentGlobalTransform = m_parentSpaceTransforms[parentIdx];
                for ( ; arrayIdx >= 0; arrayIdx-- )
                {
                    int32_t const nextIdx = boneParents[arrayIdx];
                    auto const nextTransform = m_parentSpaceTransforms[nextIdx];
                    parentGlobalTransform = nextTransform * parentGlobalTransform;
                }

                // Calculate global transform of bone
                boneGlobalTransform = boneGlobalTransform * parentGlobalTransform;
            }
        }

        return boneGlobalTransform;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Pose::DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform, Skeleton::LOD lod, Color color, float lineThickness, bool bDrawBoneNames, BoneMask const* pBoneMask, TVector<int32_t> const& boneIdxFilter ) const
    {
        auto const& parentIndices = m_pSkeleton->GetParentBoneIndices();

        //-------------------------------------------------------------------------

        auto const numBones = m_parentSpaceTransforms.size();
        if ( numBones > 0 )
        {
            // Calculate bone world transforms
            //-------------------------------------------------------------------------

            TInlineVector<Transform, 256> worldTransforms;
            worldTransforms.resize( numBones );

            worldTransforms[0] = m_parentSpaceTransforms[0] * worldTransform;
            for ( auto i = 1; i < numBones; i++ )
            {
                auto const& parentIdx = parentIndices[i];
                auto const& parentTransform = worldTransforms[parentIdx];
                worldTransforms[i] = m_parentSpaceTransforms[i] * parentTransform;
            }

            // Draw bones
            //-------------------------------------------------------------------------

            InlineString detailsStr;

            for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
            {
                auto const& parentIdx = parentIndices[boneIdx];

                if ( !boneIdxFilter.empty() && !VectorContains( boneIdxFilter, boneIdx ) )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                Color const boneColor = ( pBoneMask != nullptr ) ? BoneMask::GetColorForWeight( pBoneMask->GetWeight( boneIdx ) ) : color;

                auto const& parentTransform = worldTransforms[parentIdx];
                auto const& boneTransform = worldTransforms[boneIdx];

                ctx.DrawLine( boneTransform.GetTranslation().ToFloat3(), parentTransform.GetTranslation().ToFloat3(), boneColor, lineThickness );
                ctx.DrawAxis( boneTransform, 0.01f, 3.0f );

                if ( bDrawBoneNames )
                {
                    if ( pBoneMask != nullptr )
                    {
                        detailsStr.sprintf( "%s (%.2f)", m_pSkeleton->GetBoneID( boneIdx ).c_str(), pBoneMask->GetWeight( boneIdx ) );
                    }
                    else
                    {
                        detailsStr.sprintf( "%s", m_pSkeleton->GetBoneID( boneIdx ).c_str() );
                    }
                }
                else
                {
                    if ( pBoneMask != nullptr )
                    {
                        detailsStr.sprintf( "%.2f", pBoneMask->GetWeight( boneIdx ) );
                    }
                }

                if ( !detailsStr.empty() )
                {
                    ctx.DrawText3D( boneTransform.GetTranslation(), detailsStr.c_str(), boneColor );
                }
            }

            Skeleton::DrawRootBone( ctx, worldTransforms[0] );
        }
    }
    #endif
}