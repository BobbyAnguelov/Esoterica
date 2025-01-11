#include "ResourceLoader_IKRig.h"
#include "Engine/Animation/IK/IKRig.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    IKRigLoader::IKRigLoader()
    {
        m_loadableTypes.push_back( IKRigDefinition::GetStaticResourceTypeID() );
    }

    Resource::ResourceLoader::LoadResult IKRigLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        IKRigDefinition* pDefinition = EE::New<IKRigDefinition>();
        archive << *pDefinition;

        pResourceRecord->SetResourceData( pDefinition );
        return pDefinition->IsValid() ? Resource::ResourceLoader::LoadResult::Succeeded : Resource::ResourceLoader::LoadResult::Failed;
    }

    Resource::ResourceLoader::LoadResult IKRigLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        IKRigDefinition* pDefinition = pResourceRecord->GetResourceData<IKRigDefinition>();
        pDefinition->m_skeleton = GetInstallDependency( installDependencies, pDefinition->m_skeleton.GetResourceID() );
        pDefinition->CreateRuntimeData();
        return ResourceLoader::LoadResult::Succeeded;
    }
}