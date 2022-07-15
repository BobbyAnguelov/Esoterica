#pragma once

#include "Engine/_Module/API.h"
#include "System/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsSystem;

    //-------------------------------------------------------------------------

    class RagdollLoader final : public Resource::ResourceLoader
    {
    public:

        RagdollLoader();
        ~RagdollLoader() { EE_ASSERT( m_pPhysicsSystem == nullptr ); }

        void SetPhysicsSystemPtr( PhysicsSystem* pPhysicsSystem );
        void ClearPhysicsSystemPtr() { m_pPhysicsSystem = nullptr; }

    private:

        virtual bool LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override;
        Resource::InstallResult Install( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const override;

    private:

        PhysicsSystem* m_pPhysicsSystem = nullptr;
    };
}