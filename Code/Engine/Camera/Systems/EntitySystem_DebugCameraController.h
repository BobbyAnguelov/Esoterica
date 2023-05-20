#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntitySystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class DebugCameraComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DebugCameraController final : public EntitySystem
    {
        friend class CameraDebugView;

        EE_ENTITY_SYSTEM( DebugCameraController, RequiresUpdate( UpdateStage::PrePhysics, UpdatePriority::Highest ), RequiresUpdate( UpdateStage::Paused ) );

    private:

        virtual void RegisterComponent( EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( EntityComponent* pComponent ) override;
        virtual void Update( EntityWorldUpdateContext const& ctx ) override;

    private:

        DebugCameraComponent*           m_pCameraComponent = nullptr;
    };
}