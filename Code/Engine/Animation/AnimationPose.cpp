#include "AnimationPose.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    Pose::Pose( Skeleton const* pSkeleton, Type initialState )
        : m_pSkeleton( pSkeleton )
        , m_localTransforms( pSkeleton->GetNumBones() )
    {
        EE_ASSERT( pSkeleton != nullptr );
        Reset( initialState );
    }

    Pose::Pose( Pose&& rhs )
    {
        EE_ASSERT( rhs.m_pSkeleton != nullptr );
        operator=( eastl::move( rhs ) );
    }

    Pose& Pose::operator=( Pose&& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_localTransforms.swap( rhs.m_localTransforms );
        m_globalTransforms.swap( rhs.m_globalTransforms );
        m_state = rhs.m_state;

        return *this;
    }

    void Pose::CopyFrom( Pose const& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_localTransforms = rhs.m_localTransforms;
        m_globalTransforms = rhs.m_globalTransforms;
        m_state = rhs.m_state;
    }

    //-------------------------------------------------------------------------

    void Pose::Reset( Type initialState, bool calcGlobalPose )
    {
        switch ( initialState )
        {
            case Type::ReferencePose:
            {
                SetToReferencePose( calcGlobalPose );
            }
            break;

            case Type::ZeroPose:
            {
                SetToZeroPose( calcGlobalPose );
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
        m_localTransforms = m_pSkeleton->GetLocalReferencePose();

        if ( setGlobalPose )
        {
            m_globalTransforms = m_pSkeleton->GetGlobalReferencePose();
        }
        else
        {
            m_globalTransforms.clear();
        }

        m_state = State::ReferencePose;
    }

    void Pose::SetToZeroPose( bool setGlobalPose )
    {
        auto const numBones = m_pSkeleton->GetNumBones();
        m_localTransforms.resize( numBones, Transform::Identity );

        if ( setGlobalPose )
        {
            m_globalTransforms = m_localTransforms;
        }
        else
        {
            m_globalTransforms.clear();
        }

        m_state = State::ZeroPose;
    }

    //-------------------------------------------------------------------------

    void Pose::CalculateGlobalTransforms()
    {
        int32_t const numBones = m_pSkeleton->GetNumBones();
        m_globalTransforms.resize( numBones );

        m_globalTransforms[0] = m_localTransforms[0];
        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            int32_t const parentIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
            m_globalTransforms[boneIdx] = m_localTransforms[boneIdx] * m_globalTransforms[parentIdx];
        }
    }

    Transform Pose::GetGlobalTransform( int32_t boneIdx ) const
    {
        EE_ASSERT( boneIdx < m_pSkeleton->GetNumBones() );

        Transform boneGlobalTransform;
        if ( !m_globalTransforms.empty() )
        {
            boneGlobalTransform = m_globalTransforms[boneIdx];
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
            boneGlobalTransform = m_localTransforms[boneIdx];
            if ( nextEntry > 0 )
            {
                // Calculate global transform of parent
                int32_t arrayIdx = nextEntry - 1;
                parentIdx = boneParents[arrayIdx--];
                auto parentGlobalTransform = m_localTransforms[parentIdx];
                for ( ; arrayIdx >= 0; arrayIdx-- )
                {
                    int32_t const nextIdx = boneParents[arrayIdx];
                    auto const nextTransform = m_localTransforms[nextIdx];
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
    void Pose::DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform, Color color ) const
    {
        auto const& parentIndices = m_pSkeleton->GetParentBoneIndices();

        //-------------------------------------------------------------------------

        auto const numBones = m_localTransforms.size();
        if ( numBones > 0 )
        {
            // Calculate bone world transforms
            //-------------------------------------------------------------------------

            TInlineVector<Transform, 256> worldTransforms;
            worldTransforms.resize( numBones );

            worldTransforms[0] = m_localTransforms[0] * worldTransform;
            for ( auto i = 1; i < numBones; i++ )
            {
                auto const& parentIdx = parentIndices[i];
                auto const& parentTransform = worldTransforms[parentIdx];
                worldTransforms[i] = m_localTransforms[i] * parentTransform;
            }

            // Draw bones
            //-------------------------------------------------------------------------

            for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
            {
                auto const& parentIdx = parentIndices[boneIdx];
                auto const& parentTransform = worldTransforms[parentIdx];
                auto const& boneTransform = worldTransforms[boneIdx];

                ctx.DrawLine( boneTransform.GetTranslation().ToFloat3(), parentTransform.GetTranslation().ToFloat3(), color, 2.0f );
                ctx.DrawAxis( boneTransform, 0.03f, 3.0f );
            }

            DrawRootBone( ctx, worldTransforms[0] );
        }
    }
    #endif
}