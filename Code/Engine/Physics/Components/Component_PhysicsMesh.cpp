#include "Component_PhysicsMesh.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool PhysicsMeshComponent::HasValidPhysicsSetup() const
    {
        if ( !m_pPhysicsMesh.IsValid() )
        {
            return false;
        }

        EE_ASSERT( m_pPhysicsMesh.IsLoaded() );

        if ( !m_pPhysicsMesh->IsValid() )
        {
            EE_LOG_ERROR( "Physics", "Invalid collision mesh on Physics Mesh Component: %s (%u)", GetName().c_str(), GetID() );
            return false;
        }

        return true;
    }

    TInlineVector<StringID, 4> PhysicsMeshComponent::GetPhysicsMaterialIDs() const
    {
        TInlineVector<StringID, 4> materials = m_pPhysicsMesh->GetPhysicsMaterials();

        //-------------------------------------------------------------------------

        uint16_t const numMaterialsNeeded = m_pPhysicsMesh->GetNumMaterialsNeeded();
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

        EE_ASSERT( !m_pPhysicsMesh.IsValid() || m_pPhysicsMesh.IsLoaded() );
    }
}