#include "Animation_RuntimeGraph_Controller.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    namespace Internal
    {
        GraphControllerBase::GraphControllerBase( AnimationGraphComponent* pGraphComponent, Render::SkeletalMeshComponent* pMeshComponent )
            : m_pGraphComponent( pGraphComponent )
            , m_pAnimatedMeshComponent( pMeshComponent )
        {
            EE_ASSERT( m_pGraphComponent != nullptr && pMeshComponent != nullptr );
        }
    }

    //-------------------------------------------------------------------------

    GraphController::~GraphController()
    {
        for ( auto pController : m_subGraphControllers )
        {
            EE::Delete( pController );
        }
        m_subGraphControllers.clear();
    }

    void GraphController::PreGraphUpdate( Seconds deltaTime )
    {
        for ( auto pController : m_subGraphControllers )
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
    }
}