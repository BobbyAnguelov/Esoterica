#include "ResourceLoader_PhysicsRagdoll.h"
#include "Engine/Physics/Ragdoll/PhysicsRagdoll_Definition.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    RagdollLoader::RagdollLoader()
    {
        m_loadableTypes.push_back( RagdollDefinition::GetStaticResourceTypeID() );
    }

    Resource::LoadResult RagdollLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        RagdollDefinition* pRagdoll = EE::New<RagdollDefinition>();
        ( *pArchive ) << *pRagdoll;
        pResourceRecord->SetResourceData( pRagdoll );
        return Resource::LoadResult::Complete;
    }

    Resource::LoadResult RagdollLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        RagdollDefinition* pRagdoll = pResourceRecord->GetResourceData<RagdollDefinition>();
        pRagdoll->m_skeleton = GetInstallDependency( installDependencies, pRagdoll->m_skeleton.GetResourceID() );
        pRagdoll->Finalize();
        return pRagdoll->IsValid() ? Resource::LoadResult::Complete : Resource::LoadResult::Failed;
    }
}