#include "ResourceLoader_PhysicsCollisionMesh.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "Engine/Physics/Physics.h"
#include "Base/Serialization/BinarySerialization.h"

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

    CollisionMeshLoader::CollisionMeshLoader()
    {
        m_loadableTypes.push_back( CollisionMesh::GetStaticResourceTypeID() );
    }

    bool CollisionMeshLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        // Create collision resource
        auto pCollision = EE::New<CollisionMesh>();
        archive << *pCollision;

        // Deserialize cooked mesh data
        Blob cookedMeshData;
        archive << cookedMeshData;

        PhysXSerializedInputData cooked( cookedMeshData );
        {
            physx::PxPhysics* pPhysics = Core::GetPxPhysics();
            EE_ASSERT( pPhysics != nullptr );

            if ( pCollision->m_isConvexMesh )
            {
                auto pConvexMesh = pPhysics->createConvexMesh( cooked );
                pCollision->m_bounds = OBB( FromPx( pConvexMesh->getLocalBounds() ) );
                pCollision->m_pMesh = pConvexMesh;
            }
            else
            {
                auto pTriMesh = pPhysics->createTriangleMesh( cooked );
                pCollision->m_bounds = OBB( FromPx( pTriMesh->getLocalBounds() ) );
                pCollision->m_pMesh = pTriMesh;
            }
        }

        pResourceRecord->SetResourceData( pCollision );
        return true;
    }

    void CollisionMeshLoader::UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto pCollision = pResourceRecord->GetResourceData<CollisionMesh>();
        if ( pCollision != nullptr )
        {
            if ( pCollision->m_pMesh != nullptr )
            {
                pCollision->m_pMesh->release();
                pCollision->m_pMesh = nullptr;
            }
        }

        ResourceLoader::UnloadInternal( resID, pResourceRecord );
    }
}