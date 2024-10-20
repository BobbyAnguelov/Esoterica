#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/IK/IKRig.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINETOOLS_API IKRigResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( IKRigResourceDescriptor );
    
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return IKRigDefinition::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return IKRigDefinition::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return IKRigDefinition::s_friendlyName; }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            outDependencies.emplace_back( m_skeleton.GetResourcePath() );
        }

        virtual void Clear() override
        {
            m_skeleton.Clear();
        }

        virtual bool IsValid() const override
        {
            if ( !m_skeleton.IsSet() )
            {
                return false;
            }

            return true;
        }

    public:

        EE_REFLECT();
        TResourcePtr<Animation::Skeleton>       m_skeleton;

        EE_REFLECT( Hidden );
        TVector<IKRigDefinition::Link>          m_links;

        EE_REFLECT( );
        int32_t                                 m_iterations = 32;
    };
}