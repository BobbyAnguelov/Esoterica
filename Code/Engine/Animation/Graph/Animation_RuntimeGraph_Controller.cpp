#include "Animation_RuntimeGraph_Controller.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    namespace Internal
    {
        GraphControllerBase::GraphControllerBase( GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent )
            : m_pGraphInstance( pGraphInstance )
            , m_pAnimatedMeshComponent( pMeshComponent )
        {
            EE_ASSERT( m_pGraphInstance != nullptr && pMeshComponent != nullptr );
        }
    }

    //-------------------------------------------------------------------------

    ExternalGraphController::ExternalGraphController( StringID slotID, GraphInstance* pGraphInstance, Render::SkeletalMeshComponent* pMeshComponent, bool shouldAutoDestroy )
        : Internal::GraphControllerBase( pGraphInstance, pMeshComponent )
        , m_slotID( slotID )
        , m_requiresAutoDestruction( shouldAutoDestroy )
    {
        EE_ASSERT( slotID.IsValid() );
    }

    //-------------------------------------------------------------------------

    GraphController::GraphController( GraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent )
        : GraphControllerBase( pGraphComponent->m_pGraphInstance, pMeshComponent)
        , m_pGraphComponent( pGraphComponent )
    {}

    GraphController::~GraphController()
    {
        for ( auto pController : m_subGraphControllers )
        {
            EE::Delete( pController );
        }
        m_subGraphControllers.clear();

        //-------------------------------------------------------------------------

        for ( auto pController : m_externalGraphControllers )
        {
            // If we have any controllers that dont require auto destruction, this signifies a leak
            EE_ASSERT( pController->m_requiresAutoDestruction );
            DestroyExternalGraphController( pController );
        }
        m_externalGraphControllers.clear();
    }

    void GraphController::DestroyExternalGraphController( ExternalGraphController*& pController )
    {
        StringID const slotID = pController->m_slotID;
        EE::Delete( pController );
        m_pGraphInstance->DisconnectExternalGraph( slotID );
    }

    void GraphController::PreGraphUpdate( Seconds deltaTime )
    {
        for ( auto pController : m_subGraphControllers )
        {
            pController->PreGraphUpdate( deltaTime );
        }

        for ( auto pController : m_externalGraphControllers )
        {
            pController->PreGraphUpdate( deltaTime );
        }
    }

    void GraphController::PostGraphUpdate( Seconds deltaTime )
    {
        for ( auto pController : m_subGraphControllers )
        {
            pController->PostGraphUpdate( deltaTime );
        }

        for ( auto& pController : m_externalGraphControllers )
        {
            pController->PostGraphUpdate( deltaTime );

            // Check for auto destruction
            //-------------------------------------------------------------------------

            if ( pController->m_requiresAutoDestruction )
            {
                if ( !pController->m_wasActivated )
                {
                    pController->m_wasActivated = m_pGraphInstance->IsExternalGraphSlotNodeActive( pController->m_slotID );
                }

                bool const shouldDestroyExternalGraph = pController->m_wasActivated && !m_pGraphInstance->IsExternalGraphSlotNodeActive( pController->m_slotID );
                if ( shouldDestroyExternalGraph )
                {
                    DestroyExternalGraphController( pController );
                }
            }
        }

        // Remove all auto destroyed external controllers
        int32_t const numExternalControllers = (int32_t) m_externalGraphControllers.size();
        for ( int32_t i = numExternalControllers - 1; i >= 0; i-- )
        {
            if ( m_externalGraphControllers[i] == nullptr )
            {
                m_externalGraphControllers.erase_unsorted( m_externalGraphControllers.begin() + i );
            }
        }
    }
}