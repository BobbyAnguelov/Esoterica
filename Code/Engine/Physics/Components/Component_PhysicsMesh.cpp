#include "Component_PhysicsMesh.h"
#include "Engine/Entity/EntityLog.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool PhysicsMeshComponent::HasValidPhysicsSetup() const
    {
        if ( !m_physicsMesh.IsSet() )
        {
            return false;
        }

        EE_ASSERT( m_physicsMesh.IsLoaded() );

        if ( !m_physicsMesh->IsValid() )
        {
            EE_LOG_ENTITY_ERROR( this, "Physics", "Invalid collision mesh on Physics Mesh Component: %s (%u)", GetNameID().c_str(), GetID() );
            return false;
        }

        return true;
    }

    TInlineVector<StringID, 4> PhysicsMeshComponent::GetPhysicsMaterialIDs() const
    {
        TInlineVector<StringID, 4> materials = m_physicsMesh->GetPhysicsMaterials();

        //-------------------------------------------------------------------------

        uint16_t const numMaterialsNeeded = m_physicsMesh->GetNumMaterialsNeeded();
        for ( int32_t i = 0; i < numMaterialsNeeded; i++ )
        {
            if ( i < m_materialOverrideIDs.size() && m_materialOverrideIDs[i].IsValid() )
            {
                materials[i] = m_materialOverrideIDs[i];
            }
        }

        return materials;
    }

    void PhysicsMeshComponent::Initialize()
    {
        PhysicsShapeComponent::Initialize();

        EE_ASSERT( !m_physicsMesh.IsSet() || m_physicsMesh.IsLoaded() );
    }
}