#include "ResourceLoader_PhysicsMaterialDatabase.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------
namespace EE::Physics
{
    PhysicsMaterialDatabaseLoader::PhysicsMaterialDatabaseLoader()
    {
        m_loadableTypes.push_back( MaterialDatabase::GetStaticResourceTypeID() );
    }

    bool PhysicsMaterialDatabaseLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        MaterialDatabase* pPhysicsMaterialDB = EE::New<MaterialDatabase>();
        archive << *pPhysicsMaterialDB;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        for ( auto const& materialSettings : pPhysicsMaterialDB->m_materials )
        {
            EE_ASSERT( materialSettings.IsValid() );
        }
        #endif

        //-------------------------------------------------------------------------

        EE_ASSERT( m_pRegistry != nullptr );
        m_pRegistry->RegisterMaterials( pPhysicsMaterialDB->m_materials );
        
        //-------------------------------------------------------------------------

        pResourceRecord->SetResourceData( pPhysicsMaterialDB );
        return true;
    }

    void PhysicsMaterialDatabaseLoader::UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const
    {
        MaterialDatabase* pPhysicsMaterialDB = pResourceRecord->GetResourceData<MaterialDatabase>();
        if ( pPhysicsMaterialDB != nullptr )
        {
            EE_ASSERT( m_pRegistry != nullptr );
            m_pRegistry->UnregisterMaterials( pPhysicsMaterialDB->m_materials );
        }

        ResourceLoader::UnloadInternal( resID, pResourceRecord );
    }
}