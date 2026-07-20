#include "ResourceLoader_PhysicsMaterialDatabase.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------
namespace EE::Physics
{
    PhysicsMaterialDatabaseLoader::PhysicsMaterialDatabaseLoader()
    {
        m_loadableTypes.push_back( MaterialDatabase::GetStaticResourceTypeID() );
    }

    Resource::LoadResult PhysicsMaterialDatabaseLoader::Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const
    {
        MaterialDatabase* pPhysicsMaterialDB = EE::New<MaterialDatabase>();
        ( *pArchive ) << *pPhysicsMaterialDB;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        for ( auto const& material : pPhysicsMaterialDB->m_materials )
        {
            EE_ASSERT( material.IsValid() );
        }
        #endif

        //-------------------------------------------------------------------------

        EE_ASSERT( m_pRegistry != nullptr );
        m_pRegistry->RegisterMaterials( pPhysicsMaterialDB->m_materials );

        //-------------------------------------------------------------------------

        pResourceRecord->SetResourceData( pPhysicsMaterialDB );
        return Resource::LoadResult::Complete;
    }

    Resource::UnloadResult PhysicsMaterialDatabaseLoader::Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        MaterialDatabase* pPhysicsMaterialDB = pResourceRecord->GetResourceData<MaterialDatabase>();
        if ( pPhysicsMaterialDB != nullptr )
        {
            EE_ASSERT( m_pRegistry != nullptr );
            m_pRegistry->UnregisterMaterials( pPhysicsMaterialDB->m_materials );
        }

        return Resource::UnloadResult::Complete;
    }
}