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
        EE_REFLECT_TYPE( SkeletonResourceDescriptor );
    
        virtual bool IsValid() const override { return m_skeletonPath.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return Skeleton::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if ( m_skeletonPath.IsValid() )
            {
                outDependencies.emplace_back( m_skeletonPath );
            }
        }

    public:

        EE_REFLECT() ResourcePath                             m_skeletonPath;

        // Optional value that specifies the name of the skeleton hierarchy to use, if it is unset, we use the first skeleton we find
        EE_REFLECT() String                                   m_skeletonRootBoneName;

        // Editor-only preview mesh
        EE_REFLECT() TResourcePtr<Render::SkeletalMesh>       m_previewMesh;
    };
}