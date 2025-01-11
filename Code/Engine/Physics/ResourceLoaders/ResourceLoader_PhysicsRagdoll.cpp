#include "ResourceLoader_PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    RagdollLoader::RagdollLoader()
    {
        m_loadableTypes.push_back( RagdollDefinition::GetStaticResourceTypeID() );
    }

    Resource::ResourceLoader::LoadResult RagdollLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        RagdollDefinition* pRagdoll = EE::New<RagdollDefinition>();
        archive << *pRagdoll;

        pResourceRecord->SetResourceData( pRagdoll );
        return pRagdoll->IsValid() ? Resource::ResourceLoader::LoadResult::Succeeded : Resource::ResourceLoader::LoadResult::Failed;
    }

    Resource::ResourceLoader::LoadResult RagdollLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        RagdollDefinition* pRagdoll = pResourceRecord->GetResourceData<RagdollDefinition>();
        pRagdoll->m_skeleton = GetInstallDependency( installDependencies, pRagdoll->m_skeleton.GetResourceID() );
        pRagdoll->CreateRuntimeData();
        return ResourceLoader::LoadResult::Succeeded;
    }
}