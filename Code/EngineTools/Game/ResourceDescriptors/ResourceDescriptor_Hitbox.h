#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Hitbox/Hitbox_Definition.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Render/RenderMesh.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct EE_ENGINETOOLS_API HitboxResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( HitboxResourceDescriptor );

    public:

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return HitboxDefinition::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return HitboxDefinition::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return HitboxDefinition::s_friendlyName; }
        virtual bool RequiresInitialSetup() const override { return false; }
        virtual bool IsValid() const override;
        virtual void Clear() override;

    public:

        // Base primitive used for shape setup
        EE_REFLECT( ReadOnly );
        TResourcePtr<Animation::Skeleton>               m_skeleton;

        // Base primitive used for shape setup
        EE_REFLECT( ReadOnly );
        TResourcePtr<Render::SkeletalMesh>              m_mesh;

        //-------------------------------------------------------------------------

        EE_REFLECT( ReadOnly );
        TVector<HitboxShape>                            m_shapes;
    }; 
}