#include "ResourceLoader_PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    RagdollLoader::RagdollLoader()
    {
        m_loadableTypes.push_back( RagdollDefinition::GetStaticResourceTypeID() );
    }

    bool RagdollLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        RagdollDefinition* pRagdoll = EE::New<RagdollDefinition>();
        archive << *pRagdoll;

        pResourceRecord->SetResourceData( pRagdoll );
        return pRagdoll->IsValid();
    }

    Resource::InstallResult RagdollLoader::Install( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const
    {
        RagdollDefinition* pRagdoll = pResourceRecord->GetResourceData<RagdollDefinition>();
        pRagdoll->m_skeleton = GetInstallDependency( installDependencies, pRagdoll->m_skeleton.GetResourceID() );
        pRagdoll->CreateRuntimeData();
        return Resource::InstallResult::Succeeded;
    }
}