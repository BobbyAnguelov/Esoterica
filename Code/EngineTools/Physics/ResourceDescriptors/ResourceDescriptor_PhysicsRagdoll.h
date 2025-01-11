#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Physics/PhysicsRagdoll.h"
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

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            outDependencies.emplace_back( m_skeleton.GetDataPath() );
        }

        virtual bool IsValid() const override
        {
            if ( !m_skeleton.IsSet() )
            {
                return false;
            }

            if ( m_profiles.empty() )
            {
                return false;
            }

            // Validate the profiles
            int32_t const numBodies = (int32_t) m_bodies.size();
            for ( auto& profile : m_profiles )
            {
                if ( !profile.IsValid( numBodies ) )
                {
                    return false;
                }
            }

            return true;
        }

        virtual void Clear() override
        {
            m_skeleton.Clear();
            m_bodies.clear();
            m_profiles.clear();
        }

    public:

        EE_REFLECT();
        TResourcePtr<Animation::Skeleton>               m_skeleton;

        EE_REFLECT( ReadOnly );
        TVector<RagdollDefinition::BodyDefinition>      m_bodies;

        EE_REFLECT( ReadOnly );
        TVector<RagdollDefinition::Profile>             m_profiles;
    };
}