#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINETOOLS_API SkeletonResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( SkeletonResourceDescriptor );
    
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return Skeleton::GetStaticResourceTypeID(); }

    public:

        EE_EXPOSE ResourcePath                             m_skeletonPath;

        // Optional value that specifies the name of the skeleton hierarchy to use, if it is unset, we use the first skeleton we find
        EE_EXPOSE String                                   m_skeletonRootBoneName;

        // Editor-only preview mesh
        EE_EXPOSE TResourcePtr<Render::SkeletalMesh>       m_previewMesh;
    };
}