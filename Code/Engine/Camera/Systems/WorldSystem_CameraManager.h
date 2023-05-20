#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "System/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class CameraComponent;
    class DebugCameraComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CameraManager : public IEntityWorldSystem
    {

    public:

        EE_ENTITY_WORLD_SYSTEM( CameraManager, RequiresUpdate( UpdateStage::FrameStart, UpdatePriority::Highest ) );

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
        DebugCameraComponent* TrySpawnDebugCamera( Vector const& cameraPos = Vector::Zero, Vector const& cameraViewDir = Vector::WorldForward );
        inline DebugCameraComponent* GetDebugCamera() const { return m_pDebugCamera; }
        bool IsDebugCameraEnabled() const { return m_useDebugCamera; }
        void EnableDebugCamera( Vector const& cameraPos = Vector::Zero, Vector const& cameraViewDir = Vector::WorldForward );
        void DisableDebugCamera();
        #endif

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

        #if EE_DEVELOPMENT_TOOLS
        #endif

    private:

        TVector<CameraComponent*>                   m_cameras;
        CameraComponent*                            m_pActiveCamera = nullptr;
        int32_t                                     m_newlyAddedCamerasStartIdx = InvalidIndex;
        bool                                        m_registeredCamerasStateChanged = false;
        bool                                        m_hasAddedNonDebugCamera = false;

        #if EE_DEVELOPMENT_TOOLS
        Entity*                                     m_pDebugCameraEntity = nullptr;
        DebugCameraComponent*                       m_pDebugCamera = nullptr;
        CameraComponent*                            m_pPreviousCamera = nullptr;
        bool                                        m_useDebugCamera = false;
        bool                                        m_debugCameraWasAdded = false;
        #endif
    };
} 