#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class CameraComponent;
    class ToolsCameraComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CameraSystem : public EntityWorldSystem
    {

    public:

        EE_ENTITY_WORLD_SYSTEM( CameraSystem, RequiresUpdate( UpdateStage::GameSetup, UpdatePriority::Highest ) );

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
        ToolsCameraComponent* TrySpawnToolsCamera( Vector const& cameraPos = Vector::Zero, Vector const& cameraViewDir = Vector::WorldForward );
        inline ToolsCameraComponent* GetToolsCamera() const { return m_pToolsCamera; }
        bool IsToolsCameraEnabled() const;
        void UpdateToolsCamera( EntityWorldUpdateContext const& ctx );
        void EnableToolsCamera();
        void DisableToolsCamera();
        #endif

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

        #if EE_DEVELOPMENT_TOOLS
        #endif

    private:

        TVector<CameraComponent*>                   m_cameras;
        CameraComponent*                            m_pActiveCamera = nullptr;
        int32_t                                     m_newlyAddedCamerasStartIdx = InvalidIndex;
        bool                                        m_registeredCamerasStateChanged = false;
        bool                                        m_hasNonToolsCamera = false;

        #if EE_DEVELOPMENT_TOOLS
        Entity*                                     m_pToolsCameraEntity = nullptr;
        ToolsCameraComponent*                       m_pToolsCamera = nullptr;
        CameraComponent*                            m_pPreviousCamera = nullptr;
        bool                                        m_debugCameraWasAdded = false;
        #endif
    };
} 