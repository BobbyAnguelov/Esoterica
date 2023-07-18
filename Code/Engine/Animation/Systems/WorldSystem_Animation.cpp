#include "WorldSystem_Animation.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void AnimationWorldSystem::ShutdownSystem()
    {
        EE_ASSERT( m_graphComponents.empty() );
    }

    void AnimationWorldSystem::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pGraphComponent = TryCast<GraphComponent>( pComponent ) )
        {
            m_graphComponents.Add( pGraphComponent );
        }
    }

    void AnimationWorldSystem::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pGraphComponent = TryCast<GraphComponent>( pComponent ) )
        {
            m_graphComponents.Remove( pGraphComponent->GetID() );
        }
    }

    void AnimationWorldSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        #if EE_DEVELOPMENT_TOOLS
        Drawing::DrawContext drawingCtx = ctx.GetDrawingContext();
        for ( auto pComponent : m_graphComponents )
        {
            pComponent->DrawDebug( drawingCtx );
        }
        #endif
    }
}