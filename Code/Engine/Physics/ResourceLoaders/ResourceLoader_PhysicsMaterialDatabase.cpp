#include "ResourceLoader_PhysicsMaterialDatabase.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------
namespace EE::Physics
{
    PhysicsMaterialDatabaseLoader::PhysicsMaterialDatabaseLoader()
    {
        m_loadableTypes.push_back( PhysicsMaterialDatabase::GetStaticResourceTypeID() );
    }

    void PhysicsMaterialDatabaseLoader::SetPhysicsSystemPtr( PhysicsSystem* pPhysicsSystem )
    {
        EE_ASSERT( pPhysicsSystem != nullptr && m_pPhysicsSystem == nullptr );
        m_pPhysicsSystem = pPhysicsSystem;
    }

    bool PhysicsMaterialDatabaseLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        TVector<PhysicsMaterialSettings> serializedMaterials;
        archive << serializedMaterials;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        for ( auto const& materialSettings : serializedMaterials )
        {
            EE_ASSERT( materialSettings.IsValid() );
        }
        #endif

        //-------------------------------------------------------------------------

        m_pPhysicsSystem->FillMaterialDatabase( serializedMaterials );
        
        //-------------------------------------------------------------------------

        PhysicsMaterialDatabase* pPhysicsMaterialDB = EE::New<PhysicsMaterialDatabase>();
        pResourceRecord->SetResourceData( pPhysicsMaterialDB );
        return true;
    }

    void PhysicsMaterialDatabaseLoader::UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const
    {
        PhysicsMaterialDatabase* pPhysicsMaterialDB = pResourceRecord->GetResourceData<PhysicsMaterialDatabase>();
        if ( pPhysicsMaterialDB != nullptr )
        {
            m_pPhysicsSystem->ClearMaterialDatabase();
        }

        ResourceLoader::UnloadInternal( resID, pResourceRecord );
    }
}