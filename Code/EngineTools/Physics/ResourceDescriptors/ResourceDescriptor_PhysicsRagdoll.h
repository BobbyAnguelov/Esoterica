#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Physics/Ragdoll/PhysicsRagdoll_Definition.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    struct EE_ENGINETOOLS_API RagdollResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( RagdollResourceDescriptor );

    public:

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return RagdollDefinition::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return RagdollDefinition::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return RagdollDefinition::s_friendlyName; }
        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const override;
        virtual bool IsValid() const override;
        virtual void Clear() override;

        //-------------------------------------------------------------------------

        bool ValidateSettings() const;
        void CleanupInvalidSettings( Animation::Skeleton const* pSkeleton );


        int32_t GetNumBodies() const { return (int32_t) m_bodies.size(); }
        int32_t GetNumJoints() const { return (int32_t) m_bodies.size() - 1; }

        int32_t GetBodyIndex( StringID boneID ) const;
        int32_t GetBodyIndexForBoneID( StringID boneID ) const;
        int32_t GetBodyIndexForBoneIdx( int32_t boneIdx ) const;
        int32_t GetParentBodyIndex( Animation::Skeleton const* pSkeleton, int32_t bodyIdx ) const;
        
        Transform GetBodyTransform( Animation::Skeleton const* pSkeleton, int32_t bodyIdx ) const;
        Transform GetBodyTransform( Animation::Skeleton const* pSkeleton, StringID boneID ) const { return GetBodyTransform( pSkeleton, GetBodyIndex( boneID ) ); }

        Transform GetShapeTransform( Animation::Skeleton const* pSkeleton, int32_t bodyIdx, int32_t shapeIdx ) const;
        Transform GetShapeTransform( Animation::Skeleton const* pSkeleton, StringID boneID, int32_t shapeIdx ) const { return GetShapeTransform( pSkeleton, GetBodyIndex( boneID ), shapeIdx ); }
        void SetShapeTransform( Animation::Skeleton const* pSkeleton, int32_t bodyIdx, int32_t shapeIdx, Transform const& transform );
        void SetShapeTransform( Animation::Skeleton const* pSkeleton, StringID boneID, int32_t shapeIdx, Transform const& transform ) { SetShapeTransform( pSkeleton, GetBodyIndex( boneID ), shapeIdx, transform ); }

        RagdollShapeDefinition::Type GetShapeType( int32_t bodyIdx, int32_t shapeIdx ) const;
        RagdollShapeDefinition::Type GetShapeType( StringID boneID, int32_t shapeIdx ) const { return GetShapeType( GetBodyIndex( boneID ), shapeIdx ); }

        void ScaleShape( int32_t bodyIdx, int32_t shapeIdx, Vector const& deltaScale );
        void ScaleShape( StringID boneID, int32_t shapeIdx, Vector const& deltaScale ) { ScaleShape( GetBodyIndex( boneID ), shapeIdx, deltaScale ); }

        void EnsureUniqueDisableCollisionRules();
        bool IsCollisionDisabledBetweenBodies( int32_t bodyIdxA, int32_t bodyIdxB ) const;

    public:

        EE_REFLECT();
        TResourcePtr<Animation::Skeleton>               m_skeleton;

        EE_REFLECT( ReadOnly );
        TVector<RagdollBodyDefinition>                  m_bodies;

        EE_REFLECT( ReadOnly );
        TVector<RagdollDisableCollisionRule>            m_disableCollisionRules;
    };
}