#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "System/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationGraphComponent;

    //-------------------------------------------------------------------------

    class AnimationWorldSystem : public IEntityWorldSystem
    {
        friend class AnimationDebugView;

    public:

        EE_ENTITY_WORLD_SYSTEM( AnimationWorldSystem, RequiresUpdate( UpdateStage::FrameEnd ) );

        #if EE_DEVELOPMENT_TOOLS
        inline TVector<AnimationGraphComponent*> const& GetRegisteredGraphComponents() const { return m_graphComponents.GetVector(); }
        #endif

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TIDVector<ComponentID, AnimationGraphComponent*>          m_graphComponents;
    };
} 