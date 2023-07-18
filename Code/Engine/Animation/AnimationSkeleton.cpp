#include "AnimationSkeleton.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    bool Skeleton::IsValid() const
    {
        return !m_boneIDs.empty() && ( m_boneIDs.size() == m_parentIndices.size() ) && ( m_boneIDs.size() == m_localReferencePose.size() );
    }

    Transform Skeleton::GetBoneGlobalTransform( int32_t idx ) const
    {
        EE_ASSERT( idx >= 0 && idx < m_localReferencePose.size() );

        Transform boneGlobalTransform = m_localReferencePose[idx];
        int32_t parentIdx = GetParentBoneIndex( idx );

        while ( parentIdx != InvalidIndex )
        {
            boneGlobalTransform = boneGlobalTransform * m_localReferencePose[parentIdx];
            parentIdx = GetParentBoneIndex( parentIdx );
        }

        return boneGlobalTransform;
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

    int32_t Skeleton::GetBoneMaskIndex( StringID maskID ) const
    {
        int32_t const numMasks = (int32_t) m_boneMasks.size();
        for ( int32_t i = 0; i < numMasks; i++ )
        {
            if ( m_boneMasks[i].GetID() == maskID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    BoneMask const* Skeleton::GetBoneMask( StringID maskID ) const
    {
        BoneMask const* pMask = nullptr;
        int32_t const maskIdx = GetBoneMaskIndex( maskID );
        if ( maskIdx != InvalidIndex ) 
        {
            pMask = &m_boneMasks[maskIdx];
        }

        return pMask;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Skeleton::DrawRootBone( Drawing::DrawContext& ctx, Transform const& worldTransform )
    {
        Vector const fwdDir = worldTransform.GetForwardVector();
        Vector const upDir = worldTransform.GetUpVector();
        Vector const rightDir = worldTransform.GetRightVector();
        
        Vector const charPos = worldTransform.GetTranslation();

        static constexpr float const forwardAxisLength = 0.1f;
        static constexpr float const axisLength = 0.05f;

        ctx.DrawLine( charPos, charPos, Colors::Yellow, 2 );
        ctx.DrawArrow( charPos, charPos + fwdDir * forwardAxisLength, Colors::Lime, 4 );
        ctx.DrawLine( charPos, charPos + upDir * axisLength, Colors::Blue, 3 );
        ctx.DrawLine( charPos, charPos + rightDir * axisLength, Colors::Red, 3 );
        ctx.DrawPoint( charPos, Colors::HotPink, 10 );
    }

    void Skeleton::DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform ) const
    {
        auto const numBones = m_localReferencePose.size();
        if ( numBones > 0 )
        {
            TInlineVector<Transform, 256> globalTransforms;
            globalTransforms.resize( numBones );

            globalTransforms[0] = m_localReferencePose[0] * worldTransform;
            for ( auto i = 1; i < numBones; i++ )
            {
                auto const& parentIdx = m_parentIndices[i];
                auto const& parentTransform = globalTransforms[parentIdx];
                globalTransforms[i] = m_localReferencePose[i] * parentTransform;
            }

            //-------------------------------------------------------------------------

            for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
            {
                auto const& parentIdx = m_parentIndices[boneIdx];
                auto const& parentTransform = globalTransforms[parentIdx];
                auto const& boneTransform = globalTransforms[boneIdx];

                ctx.DrawLine( boneTransform.GetTranslation().ToFloat3(), parentTransform.GetTranslation().ToFloat3(), Colors::HotPink, 3.0f );
                ctx.DrawAxis( boneTransform, 0.03f, 2.0f );
            }

            DrawRootBone( ctx, globalTransforms[0] );
        }
    }
    #endif
}