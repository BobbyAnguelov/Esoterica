#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "Base/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphComponent;

    //-------------------------------------------------------------------------

    class AnimationWorldSystem : public EntityWorldSystem
    {
        friend class AnimationDebugView;

    public:

        EE_ENTITY_WORLD_SYSTEM( AnimationWorldSystem, RequiresUpdate( UpdateStage::FrameEnd ) );

        #if EE_DEVELOPMENT_TOOLS
        inline TVector<GraphComponent*> const& GetRegisteredGraphComponents() const { return m_graphComponents.GetVector(); }
        #endif

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TIDVector<ComponentID, GraphComponent*>          m_graphComponents;
    };
} 