#include "ResourceDescriptor_PhysicsRagdoll.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    void RagdollResourceDescriptor::GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const
    {
        outDependencies.emplace_back( m_skeleton.GetDataPath(), true );
    }

    bool RagdollResourceDescriptor::IsValid() const
    {
        if ( !m_skeleton.IsSet() )
        {
            return false;
        }

        return true;
    }

    void RagdollResourceDescriptor::Clear()
    {
        m_skeleton.Clear();
        m_bodies.clear();
    }

    //-------------------------------------------------------------------------

    int32_t RagdollResourceDescriptor::GetBodyIndex( StringID boneID ) const
    {
        for ( int32_t i = 0; i < m_bodies.size(); i++ )
        {
            if ( m_bodies[i].m_boneID == boneID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    int32_t RagdollResourceDescriptor::GetBodyIndexForBoneID( StringID boneID ) const
    {
        for ( auto bodyIdx = 0; bodyIdx < m_bodies.size(); bodyIdx++ )
        {
            if ( m_bodies[bodyIdx].m_boneID == boneID )
            {
                return bodyIdx;
            }
        }

        return InvalidIndex;
    }

    int32_t RagdollResourceDescriptor::GetBodyIndexForBoneIdx( int32_t boneIdx ) const
    {
        auto const& boneID = m_skeleton->GetBoneID( boneIdx );
        return GetBodyIndexForBoneID( boneID );
    }

    int32_t RagdollResourceDescriptor::GetParentBodyIndex( Animation::Skeleton const* pSkeleton, int32_t bodyIdx ) const
    {
        EE_ASSERT( pSkeleton != nullptr );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < m_bodies.size() );
        int32_t const boneIdx = pSkeleton->GetBoneIndex( m_bodies[bodyIdx].m_boneID );

        // Traverse the hierarchy to try to find the first parent bone that has a body
        int32_t parentBoneIdx = pSkeleton->GetParentBoneIndex( boneIdx );
        while ( parentBoneIdx != InvalidIndex )
        {
            // Do we have a body for this parent bone?
            StringID const parentBoneID = pSkeleton->GetBoneID( parentBoneIdx );
            for ( int32_t i = 0; i < m_bodies.size(); i++ )
            {
                if ( m_bodies[i].m_boneID == parentBoneID )
                {
                    return i;
                }
            }

            parentBoneIdx = pSkeleton->GetParentBoneIndex( parentBoneIdx );
        }

        return InvalidIndex;
    }

    //-------------------------------------------------------------------------

    Transform RagdollResourceDescriptor::GetBodyTransform( Animation::Skeleton const* pSkeleton, int32_t bodyIdx ) const
    {
        EE_ASSERT( pSkeleton != nullptr );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < m_bodies.size() );
        int32_t const boneIdx = pSkeleton->GetBoneIndex( m_bodies[bodyIdx].m_boneID );
        EE_ASSERT( boneIdx != InvalidIndex );
        return pSkeleton->GetBoneModelSpaceTransform( boneIdx );
    }

    Transform RagdollResourceDescriptor::GetShapeTransform( Animation::Skeleton const* pSkeleton, int32_t bodyIdx, int32_t shapeIdx ) const
    {
        EE_ASSERT( pSkeleton != nullptr );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < m_bodies.size() );
        EE_ASSERT( shapeIdx >= 0 && shapeIdx < m_bodies[bodyIdx].m_shapes.size() );

        int32_t const boneIdx = pSkeleton->GetBoneIndex( m_bodies[bodyIdx].m_boneID );
        EE_ASSERT( boneIdx != InvalidIndex );

        Transform const bodyTransform = pSkeleton->GetBoneModelSpaceTransform( boneIdx );
        Transform const shapeTransform = m_bodies[bodyIdx].m_shapes[shapeIdx].m_offsetTransform * bodyTransform;
        return shapeTransform;
    }

    void RagdollResourceDescriptor::SetShapeTransform( Animation::Skeleton const* pSkeleton, int32_t bodyIdx, int32_t shapeIdx, Transform const& shapeTransform )
    {
        EE_ASSERT( pSkeleton != nullptr );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < m_bodies.size() );
        EE_ASSERT( shapeIdx >= 0 && shapeIdx < m_bodies[bodyIdx].m_shapes.size() );

        int32_t const boneIdx = pSkeleton->GetBoneIndex( m_bodies[bodyIdx].m_boneID );
        EE_ASSERT( boneIdx != InvalidIndex );

        Transform const bodyTransform = pSkeleton->GetBoneModelSpaceTransform( boneIdx );
        m_bodies[bodyIdx].m_shapes[shapeIdx].m_offsetTransform = shapeTransform * bodyTransform.GetInverse();
    }

    RagdollShapeDefinition::Type RagdollResourceDescriptor::GetShapeType( int32_t bodyIdx, int32_t shapeIdx ) const
    {
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < m_bodies.size() );
        EE_ASSERT( shapeIdx >= 0 && shapeIdx < m_bodies[bodyIdx].m_shapes.size() );
        return m_bodies[bodyIdx].m_shapes[shapeIdx].m_type;
    }

    void RagdollResourceDescriptor::ScaleShape( int32_t bodyIdx, int32_t shapeIdx, Vector const& deltaScale )
    {
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < m_bodies.size() );
        EE_ASSERT( shapeIdx >= 0 && shapeIdx < m_bodies[bodyIdx].m_shapes.size() );

        auto& shape = m_bodies[bodyIdx].m_shapes[shapeIdx];

        if ( shape.m_type == RagdollShapeDefinition::Type::Box )
        {
            shape.m_boxExtents *= ( Vector::One + deltaScale );
        }
        else if ( shape.m_type == RagdollShapeDefinition::Type::Sphere )
        {
            shape.m_radius *= ( 1.0f + deltaScale.GetX() );
        }
        else if ( shape.m_type == RagdollShapeDefinition::Type::Capsule )
        {
            float const halfHeightScale = deltaScale.GetZ();
            if ( !Math::IsNearZero( halfHeightScale ) )
            {
                shape.m_halfHeight *= ( 1.0f + halfHeightScale );
                shape.m_halfHeight = Math::Max( shape.m_halfHeight, 0.01f );
            }

            float const radiusScale = Math::Abs( deltaScale.GetX() ) > Math::Abs( deltaScale.GetY() ) ? deltaScale.GetX() : deltaScale.GetY();
            if ( !Math::IsNearZero( radiusScale ) )
            {
                shape.m_radius *= ( 1.0f + radiusScale );
                shape.m_radius = Math::Max( shape.m_radius, 0.01f );
            }
        }
    }

    //-------------------------------------------------------------------------

    bool RagdollResourceDescriptor::ValidateSettings() const
    {
        for ( auto const& body : m_bodies )
        {
            if ( !body.IsValid() )
            {
                return false;
            }
        }

        for ( auto const& rule : m_disableCollisionRules )
        {
            if ( !rule.IsValid() )
            {
                return false;
            }
        }

        return true;
    }

    void RagdollResourceDescriptor::CleanupInvalidSettings( Animation::Skeleton const* pSkeleton )
    {
        for ( int32_t i = (int32_t) m_bodies.size() - 1; i >= 0; i-- )
        {
            if ( !m_bodies[i].IsValid() || pSkeleton->GetBoneIndex( m_bodies[i].m_boneID ) == InvalidIndex )
            {
                m_bodies.erase( m_bodies.begin() + i );
            }
        }

        for ( int32_t i = (int32_t) m_disableCollisionRules.size() - 1; i >=0; i-- )
        {
            if ( !m_disableCollisionRules[i].IsValid() )
            {
                m_disableCollisionRules.erase( m_disableCollisionRules.begin() + i );
            }
        }

        EnsureUniqueDisableCollisionRules();
    }

    void RagdollResourceDescriptor::EnsureUniqueDisableCollisionRules()
    {
        // Ensure unique rules
        TVector<RagdollDisableCollisionRule> oldRules;
        oldRules.swap( m_disableCollisionRules );

        for ( auto const& rule : oldRules )
        {
            VectorEmplaceBackUnique( m_disableCollisionRules, rule );
        }
    }

    bool RagdollResourceDescriptor::IsCollisionDisabledBetweenBodies( int32_t bodyIdxA, int32_t bodyIdxB ) const
    {
        RagdollDisableCollisionRule rule;
        rule.m_boneA = m_bodies[bodyIdxA].m_boneID;
        rule.m_boneB = m_bodies[bodyIdxB].m_boneID;
        return VectorContains( m_disableCollisionRules, rule );
    }
}