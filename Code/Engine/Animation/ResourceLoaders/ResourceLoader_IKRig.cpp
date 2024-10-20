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

    bool IKRigLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        IKRigDefinition* pDefinition = EE::New<IKRigDefinition>();
        archive << *pDefinition;

        pResourceRecord->SetResourceData( pDefinition );
        return pDefinition->IsValid();
    }

    Resource::InstallResult IKRigLoader::Install( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        IKRigDefinition* pDefinition = pResourceRecord->GetResourceData<IKRigDefinition>();
        pDefinition->m_skeleton = GetInstallDependency( installDependencies, pDefinition->m_skeleton.GetResourceID() );
        pDefinition->CreateRuntimeData();
        return Resource::InstallResult::Succeeded;
    }
}