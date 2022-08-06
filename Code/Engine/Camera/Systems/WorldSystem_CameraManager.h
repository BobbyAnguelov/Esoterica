#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class CameraComponent;
    class DebugCameraComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CameraManager : public IEntityWorldSystem
    {

    public:

        EE_REGISTER_ENTITY_WORLD_SYSTEM( CameraManager, RequiresUpdate( UpdateStage::FrameStart, UpdatePriority::Highest ) );

        //-------------------------------------------------------------------------

        // Do we have an active camera set
        inline bool HasActiveCamera() const { return m_pActiveCamera != nullptr; }

        // Get the current active camera
        inline CameraComponent* GetActiveCamera() const { return m_pActiveCamera; }

        // Switch the active camera - it needs to be an already registered camera!
        void SetActiveCamera( CameraComponent const* pCamera );

        // Debug Camera
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        bool HasDebugCamera() const { return m_pDebugCamera != nullptr; }
        bool IsDebugCameraEnabled() const;
        inline DebugCameraComponent* GetDebugCamera() const { return m_pDebugCamera; }
        #endif

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TVector<CameraComponent*>                   m_cameras;
        CameraComponent*                            m_pActiveCamera = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        DebugCameraComponent*                       m_pDebugCamera = nullptr;
        #endif
    };
} 