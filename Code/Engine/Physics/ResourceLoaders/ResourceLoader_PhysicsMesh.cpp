#include "ResourceLoader_PhysicsMesh.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Physics/PhysicsMesh.h"
#include "Engine/Physics/PhysX.h"
#include "System/Serialization/BinarySerialization.h"

#include <PxPhysics.h>
#include <foundation/PxIO.h>

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------
namespace EE::Physics
{
    class PhysXSerializedInputData : public physx::PxInputStream
    {
    public:

        PhysXSerializedInputData( Blob const& buffer ) : m_buffer( buffer ) {}

    private:

        virtual PxU32 read( void* dest, PxU32 count ) override final
        {
            EE_ASSERT( dest != nullptr );
            memcpy( dest, &m_buffer[m_readByteIdx], count );
            m_readByteIdx += count;
            return count;
        }

    private:

        Blob const&     m_buffer;
        size_t          m_readByteIdx = 0;
    };

    //-------------------------------------------------------------------------

    PhysicsMeshLoader::PhysicsMeshLoader()
    {
        m_loadableTypes.push_back( PhysicsMesh::GetStaticResourceTypeID() );
    }

    void PhysicsMeshLoader::SetPhysicsSystemPtr( PhysicsSystem* pPhysicsSystem )
    {
        EE_ASSERT( pPhysicsSystem != nullptr && m_pPhysicsSystem == nullptr );
        m_pPhysicsSystem = pPhysicsSystem;
    }

    bool PhysicsMeshLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        // Create mesh resource
        PhysicsMesh* pPhysicsMesh = EE::New<PhysicsMesh>();
        archive << *pPhysicsMesh;

        // Deserialize cooked mesh data
        Blob cookedMeshData;
        archive << cookedMeshData;

        PhysXSerializedInputData cooked( cookedMeshData );
        {
            physx::PxPhysics* pPhysics = m_pPhysicsSystem->GetPxPhysics();

            if ( pPhysicsMesh->m_isConvexMesh )
            {
                auto pConvexMesh = pPhysics->createConvexMesh( cooked );
                pPhysicsMesh->m_bounds = OBB( FromPx( pConvexMesh->getLocalBounds() ) );
                pPhysicsMesh->m_pMesh = pConvexMesh;
            }
            else
            {
                auto pTriMesh = pPhysics->createTriangleMesh( cooked );
                pPhysicsMesh->m_bounds = OBB( FromPx( pTriMesh->getLocalBounds() ) );
                pPhysicsMesh->m_pMesh = pTriMesh;
            }
        }

        pResourceRecord->SetResourceData( pPhysicsMesh );
        return true;
    }

    void PhysicsMeshLoader::UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const
    {
        PhysicsMesh* pPhysicsGeometry = pResourceRecord->GetResourceData<PhysicsMesh>();
        if ( pPhysicsGeometry != nullptr )
        {
            if ( pPhysicsGeometry->m_pMesh != nullptr )
            {
                pPhysicsGeometry->m_pMesh->release();
                pPhysicsGeometry->m_pMesh = nullptr;
            }
        }

        ResourceLoader::UnloadInternal( resID, pResourceRecord );
    }
}