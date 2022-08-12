#include "Component_StaticMesh.h"
#include "Engine/Entity/EntityLog.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    TEvent<StaticMeshComponent*> StaticMeshComponent::s_staticMobilityTransformUpdatedEvent;
    TEvent<StaticMeshComponent*> StaticMeshComponent::s_mobilityChangedEvent;

    //-------------------------------------------------------------------------

    void StaticMeshComponent::Initialize()
    {
        MeshComponent::Initialize();

        if ( HasMeshResourceSet() )
        {
            SetLocalBounds( m_pMesh->GetBounds() );
        }
    }

    void StaticMeshComponent::ChangeMobility( Mobility newMobility )
    {
        if ( newMobility != m_mobility )
        {
            m_mobility = newMobility;
            s_mobilityChangedEvent.Execute( this );
        }
    }

    void StaticMeshComponent::OnWorldTransformUpdated()
    {
        if( IsInitialized() )
        {
            if ( m_mobility == Mobility::Static )
            {
                s_staticMobilityTransformUpdatedEvent.Execute( this );
            }
        }
    }

    TVector<TResourcePtr<Render::Material>> const& StaticMeshComponent::GetDefaultMaterials() const
    {
        EE_ASSERT( IsInitialized() && HasMeshResourceSet() );
        return m_pMesh->GetMaterials();
    }
}