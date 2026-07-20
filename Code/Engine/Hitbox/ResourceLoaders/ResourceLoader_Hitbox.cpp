#include "ResourceLoader_Hitbox.h"
#include "Engine/Hitbox/Hitbox_Definition.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    HitboxLoader::HitboxLoader()
    {
        m_loadableTypes.push_back( HitboxDefinition::GetStaticResourceTypeID() );
    }

    Resource::LoadResult HitboxLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        HitboxDefinition* pHitbox = EE::New<HitboxDefinition>();
        ( *pArchive ) << *pHitbox;
        pResourceRecord->SetResourceData( pHitbox );

        return Resource::LoadResult::Complete;
    }

    Resource::LoadResult HitboxLoader::Install( ResourceID const& resourceID, Resource::InstallDependencyList const& installDependencies, Resource::ResourceRecord* pResourceRecord ) const
    {
        HitboxDefinition* pHitbox = pResourceRecord->GetResourceData<HitboxDefinition>();
        return pHitbox->IsValid() ? Resource::LoadResult::Complete : Resource::LoadResult::Failed;
    }
}