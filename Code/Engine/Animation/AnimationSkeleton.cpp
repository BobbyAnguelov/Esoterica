#include "AnimationSkeleton.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    bool Skeleton::ValidateSkeletonSetup( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons )
    {
        if ( pPrimarySkeleton == nullptr )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        int32_t const numSkeletons = (int32_t) secondarySkeletons.size();
        for ( auto i = 0; i < numSkeletons; i++ )
        {
            if ( secondarySkeletons[i] == nullptr )
            {
                return false;
            }

            if ( secondarySkeletons[i] == pPrimarySkeleton )
            {
                return false;
            }

            for ( auto j = i + 1; j < numSkeletons; j++ )
            {
                if ( secondarySkeletons[j] == secondarySkeletons[i] )
                {
                    return false;
                }
            }
        }

        return true;
    }
    #endif

    //-------------------------------------------------------------------------

    bool Skeleton::IsValid() const
    {
        return !m_boneIDs.empty() && ( m_boneIDs.size() == m_parentIndices.size() ) && ( m_boneIDs.size() == m_parentSpaceReferencePose.size() );
    }

    int32_t Skeleton::GetFirstChildBoneIndex( int32_t boneIdx ) const
    {
        int32_t const numBones = GetNumBones();
        EE_ASSERT( IsValidBoneIndex( boneIdx ) );

        int32_t childIdx = InvalidIndex;
        for ( auto i = boneIdx + 1; i < numBones; i++ )
        {
            if ( m_parentIndices[i] == boneIdx )
            {
                childIdx = i;
                break;
            }
        }

        return childIdx;
    }

    bool Skeleton::IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const
    {
        EE_ASSERT( IsValidBoneIndex( parentBoneIdx ) );
        EE_ASSERT( IsValidBoneIndex( childBoneIdx ) );

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

    int32_t Skeleton::GetFloatChannelSetIndex( StringID ID ) const
    {
        for ( int32_t i = 0; i < m_floatChannelSets.size(); i++ )
        {
            if ( m_floatChannelSets[i].m_ID == ID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    FloatChannelSet const *Skeleton::GetFloatChannelSet( StringID channelSetID ) const
    {
        FloatChannelSet const *pCurvesSet = nullptr;
        int32_t const setIdx = GetFloatChannelSetIndex( channelSetID );
        if ( setIdx != InvalidIndex )
        {
            pCurvesSet = &m_floatChannelSets[setIdx];
        }

        return pCurvesSet;
    }

    int8_t Skeleton::GetBoneMaskSetIndex( StringID maskSetID ) const
    {
        EE_ASSERT( m_boneMaskSets.size() < INT8_MAX );
        int8_t const numMasks = (int8_t) m_boneMaskSets.size();
        for ( int8_t i = 0; i < numMasks; i++ )
        {
            if ( m_boneMaskSets[i].m_ID == maskSetID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    BoneMaskSet const* Skeleton::GetBoneMaskSet( StringID maskSetID ) const
    {
        BoneMaskSet const* pMaskSet = nullptr;
        int32_t const maskSetIdx = GetBoneMaskSetIndex( maskSetID );
        if ( maskSetIdx != InvalidIndex )
        {
            pMaskSet = &m_boneMaskSets[maskSetIdx];
        }

        return pMaskSet;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Skeleton::DrawRootBone( DebugDrawContext& ctx, Transform const& worldTransform )
    {
        Vector const fwdDir = worldTransform.GetForwardVector();
        Vector const upDir = worldTransform.GetUpVector();
        
        Vector const charPos = worldTransform.GetTranslation();

        static constexpr float const forwardAxisLength = 0.1f;
        static constexpr float const axisLength = 0.05f;

        ctx.DrawLine( charPos, charPos, Colors::Yellow, 2 );
        ctx.DrawArrow( charPos, charPos + fwdDir * forwardAxisLength, Colors::Cyan, 4 );
        ctx.DrawLine( charPos, charPos + upDir * axisLength, Colors::Blue, 3 );
        ctx.DrawPoint( charPos, Colors::HotPink, 10 );
    }

    void Skeleton::DrawDebug( DebugDrawContext& ctx, Transform const& worldTransform, LOD lod, DrawOptions const& options ) const
    {
        bool const hasWeights = options.m_pBoneWeights != nullptr;
        if ( hasWeights )
        {
            EE_ASSERT( options.m_pBoneWeights->size() >= m_boneIDs.size() );
        }

        //-------------------------------------------------------------------------

        InlineString detailsStr;
        size_t const numBones = GetNumBones( lod );
        if ( numBones > 0 )
        {
            TInlineVector<Transform, 256> modelSpaceTransforms;
            modelSpaceTransforms.resize( numBones );

            modelSpaceTransforms[0] = m_parentSpaceReferencePose[0] * worldTransform;
            for ( auto i = 1; i < numBones; i++ )
            {
                auto const& parentIdx = m_parentIndices[i];
                auto const& parentTransform = modelSpaceTransforms[parentIdx];
                modelSpaceTransforms[i] = m_parentSpaceReferencePose[i] * parentTransform;
            }

            //-------------------------------------------------------------------------

            for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
            {
                auto const& parentIdx = m_parentIndices[boneIdx];
                auto const& parentTransform = modelSpaceTransforms[parentIdx];
                auto const& boneTransform = modelSpaceTransforms[boneIdx];

                float const boneWeight = hasWeights ? ( *options.m_pBoneWeights )[boneIdx] : 1.0f;
                Color boneColor = hasWeights ? GetColorForBoneWeight( boneWeight ) : options.m_boneColor;

                bool bIsFilteredOut = options.IsBoneFilteredOut( boneIdx );
                if ( bIsFilteredOut )
                {
                    boneColor.m_byteColor.m_a = 25;
                }

                ctx.DrawLine( boneTransform.GetTranslation().ToFloat3(), parentTransform.GetTranslation().ToFloat3(), boneColor, options.m_lineThickness );

                if ( !bIsFilteredOut && options.m_drawAxes )
                {
                    ctx.DrawAxis( boneTransform, 0.02f, 2.0f );
                }

                if ( !bIsFilteredOut && options.m_drawBoneNames )
                {
                    if ( hasWeights )
                    {
                        detailsStr.sprintf( "%s (%.2f)", GetBoneID( boneIdx ).c_str(), boneWeight );
                    }
                    else
                    {
                        detailsStr.sprintf( "%s", GetBoneID( boneIdx ).c_str() );
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

            DrawRootBone( ctx, modelSpaceTransforms[0] );
        }
    }

    StringID Skeleton::GetPreviewAttachmentBoneIDForSecondarySkeleton( Skeleton const* pSkeleton ) const
    {
        for ( auto const& secondarySkeletonRef : m_secondarySkeletons )
        {
            if ( secondarySkeletonRef.m_skeleton.GetPtr<Skeleton>() == pSkeleton )
            {
                return secondarySkeletonRef.m_attachmentBoneID;
            }
        }

        return StringID();
    }
    #endif
}