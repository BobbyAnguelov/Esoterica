#include "AnimationPose.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    Pose::Pose( Skeleton const* pSkeleton, Init initialState )
        : m_pSkeleton( pSkeleton )
        , m_parentSpaceTransforms( pSkeleton->GetNumBones() )
    {
        EE_ASSERT( pSkeleton != nullptr );
        Reset( initialState );

        //-------------------------------------------------------------------------

        m_floatChannelSetValues.reserve( pSkeleton->GetNumFloatChannelSets() );
        for ( auto const &set : pSkeleton->GetFloatChannelSets() )
        {
            m_floatChannelSetValues.emplace_back( &set );
        }
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
        m_floatChannelSetValues.swap( rhs.m_floatChannelSetValues );
        m_state = rhs.m_state;

        return *this;
    }

    Pose& Pose::operator=( Pose const& rhs )
    {
        CopyFrom( rhs );
        return *this;
    }

    void Pose::CopyFrom( Pose const& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_parentSpaceTransforms = rhs.m_parentSpaceTransforms;
        m_modelSpaceTransforms = rhs.m_modelSpaceTransforms;
        m_floatChannelSetValues = rhs.m_floatChannelSetValues;
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
        m_floatChannelSetValues.swap( rhs.m_floatChannelSetValues );
    }

    void Pose::ChangeSkeleton( Skeleton const* pSkeleton )
    {
        EE_ASSERT( pSkeleton != nullptr );

        if ( m_pSkeleton == pSkeleton )
        {
            return;
        }

        m_pSkeleton = pSkeleton;
        m_parentSpaceTransforms.resize( pSkeleton->GetNumBones() );
        m_modelSpaceTransforms.clear();
        m_state = State::Unset;

        //-------------------------------------------------------------------------

        m_floatChannelSetValues.clear();
        m_floatChannelSetValues.reserve( pSkeleton->GetNumFloatChannelSets() );
        for ( auto const &set : pSkeleton->GetFloatChannelSets() )
        {
            m_floatChannelSetValues.emplace_back( &set );
        }
    }

    //-------------------------------------------------------------------------

    void Pose::Reset( Init initialState, bool calculateModelSpacePose )
    {
        switch ( initialState )
        {
            case Init::ReferencePose:
            {
                SetToReferencePose( calculateModelSpacePose );
            }
            break;

            case Init::ZeroPose:
            {
                SetToZeroPose( calculateModelSpacePose );
            }
            break;

            default:
            {
                // Leave memory intact, just change state
                m_state = State::Unset;

                // Reset all float channels
                for ( auto& v : m_floatChannelSetValues )
                {
                    v.Reset();
                }
            }
            break;
        }
    }

    void Pose::SetToReferencePose( bool setGlobalPose )
    {
        m_parentSpaceTransforms = m_pSkeleton->GetParentSpaceReferencePose();

        if ( setGlobalPose )
        {
            m_modelSpaceTransforms = m_pSkeleton->GetModelSpaceReferencePose();
        }
        else
        {
            m_modelSpaceTransforms.clear();
        }

        // Reset all float channels
        for ( auto& v : m_floatChannelSetValues )
        {
            v.Reset();
        }

        m_state = State::ReferencePose;
    }

    void Pose::SetToZeroPose( bool setGlobalPose, bool setZeroScale )
    {
        auto const numBones = m_pSkeleton->GetNumBones();
        m_parentSpaceTransforms.clear();
        m_parentSpaceTransforms.resize( numBones, setZeroScale ? Transform::Zero : Transform::Identity );

        if ( setGlobalPose )
        {
            m_modelSpaceTransforms = m_parentSpaceTransforms;
        }
        else
        {
            m_modelSpaceTransforms.clear();
        }

        // Reset all float channels
        for ( auto& v : m_floatChannelSetValues )
        {
            v.Reset();
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

    Transform Pose::GetModelSpaceTransform( int32_t boneIdx ) const
    {
        EE_ASSERT( boneIdx != InvalidIndex && boneIdx < m_pSkeleton->GetNumBones() );

        Transform boneModelSpaceTransform;
        if ( !m_modelSpaceTransforms.empty() )
        {
            boneModelSpaceTransform = m_modelSpaceTransforms[boneIdx];
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
            boneModelSpaceTransform = m_parentSpaceTransforms[boneIdx];
            if ( nextEntry > 0 )
            {
                // Calculate global transform of parent
                int32_t arrayIdx = nextEntry - 1;
                parentIdx = boneParents[arrayIdx--];
                Transform parentModelSpaceTransform = m_parentSpaceTransforms[parentIdx];
                for ( ; arrayIdx >= 0; arrayIdx-- )
                {
                    int32_t const nextIdx = boneParents[arrayIdx];
                    Transform const& nextTransform = m_parentSpaceTransforms[nextIdx];
                    parentModelSpaceTransform = nextTransform * parentModelSpaceTransform;
                }

                // Calculate global transform of bone
                boneModelSpaceTransform = boneModelSpaceTransform * parentModelSpaceTransform;
            }
        }

        return boneModelSpaceTransform;
    }

    TInlineVector<BoneChainElement, 20> Pose::CalculateModelSpaceTransformsForChain( int32_t chainEndBoneIdx ) const
    {
        EE_ASSERT( chainEndBoneIdx >= 0 && chainEndBoneIdx < GetNumBones() );

        // Get the entire chain
        TInlineVector<int32_t, 100> boneChainIndices;
        int32_t boneIdx = chainEndBoneIdx;
        while ( boneIdx != InvalidIndex )
        {
            boneChainIndices.emplace_back( boneIdx );
            boneIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
        }
        EE_ASSERT( boneChainIndices.back() == 0 );

        // Fill out chain
        TInlineVector<BoneChainElement, 20> chain;
        int32_t const numBonesInChain = int32_t( boneChainIndices.size() );
        for ( int32_t i = numBonesInChain - 1; i >= 0; i-- )
        {
            boneIdx = boneChainIndices[i];
            chain.emplace_back( boneIdx, m_pSkeleton->GetBoneID( boneIdx ), m_parentSpaceTransforms[boneIdx], m_parentSpaceTransforms[boneIdx] );
        }

        // Calculate model space transforms
        for ( int32_t i = 1; i < numBonesInChain; i++ )
        {
            chain[i].m_modelSpaceTransform = chain[i].m_parentSpaceTransform * chain[i-1].m_modelSpaceTransform;
        }

        return chain;
    }

    //-------------------------------------------------------------------------

    bool Pose::HasFloatChannelValues() const
    {
        for ( auto const& v : m_floatChannelSetValues )
        {
            if ( v.IsSet() )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Pose::DrawDebug( DebugDrawContext& ctx, Transform const& worldTransform, Skeleton::LOD lod, DrawOptions const& options ) const
    {
        auto const& parentIndices = m_pSkeleton->GetParentBoneIndices();

        //-------------------------------------------------------------------------

        if ( !IsPoseSet() )
        {
            m_pSkeleton->DrawDebug( ctx, worldTransform, lod, options );
            return;
        }

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

                //-------------------------------------------------------------------------

                bool const hasWeights = options.m_pBoneWeights != nullptr;
                float const boneWeight = hasWeights ? ( *options.m_pBoneWeights )[boneIdx] : 1.0f;
                Color boneColor = hasWeights ? Color::EvaluateRedGreenGradient( boneWeight ) : options.m_boneColor;

                bool bIsFilteredOut = options.IsBoneFilteredOut( boneIdx );
                if ( bIsFilteredOut )
                {
                    boneColor.m_byteColor.m_a = 25;
                }

                auto const& parentTransform = worldTransforms[parentIdx];
                auto const& boneTransform = worldTransforms[boneIdx];

                ctx.DrawLine( boneTransform.GetTranslation().ToFloat3(), parentTransform.GetTranslation().ToFloat3(), boneColor, options.m_lineThickness );

                if ( !bIsFilteredOut && options.m_drawAxes )
                {
                    ctx.DrawAxis( boneTransform, options.m_axisLength, options.m_axisThickness );
                }

                if ( !bIsFilteredOut && options.m_drawBoneNames )
                {
                    if ( hasWeights )
                    {
                        detailsStr.sprintf( "%s (%.2f)", m_pSkeleton->GetBoneID( boneIdx ).c_str(), boneWeight );
                    }
                    else
                    {
                        detailsStr.sprintf( "%s", m_pSkeleton->GetBoneID( boneIdx ).c_str() );
                    }
                }
                else
                {
                    if ( hasWeights )
                    {
                        detailsStr.sprintf( "%.2f", boneWeight );
                    }
                }

                if ( !detailsStr.empty() )
                {
                    ctx.DrawText3D( boneTransform.GetTranslation(), detailsStr.c_str(), boneColor, DebugFont::Small );
                }
            }

            Skeleton::DrawRootBone( ctx, worldTransforms[0] );
        }
    }
    #endif
}