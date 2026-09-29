#pragma once

#include "Engine/Entity/EntityWorldSystem.h"
#include "Engine/Entity/EntityWorldSystemSignal.h"

#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/Device/DeviceResizeBuffer.h"
#include "Engine/Viewport/ViewportPicking.h"

#include "Base/Resource/ResourcePtr.h"
#include "Base/Render/RHI.h"
#include "Base/Math/ViewVolume.h"
#include "Base/Systems.h"
#include "Base/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshComponent;
    class SkeletalMeshComponent;
    class PCGComponent;
    class DirectionalLightComponent;
    class PointLightComponent;
    class SpotLightComponent;
    class GlobalEnvironmentMapComponent;
    class LocalEnvironmentMapComponent;
    class Mesh;
    class Material;
    class RenderSystem;
    class RenderViewport;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RenderWorldSystem final : public EntityWorldSystem
    {
        friend class ForwardShadingRenderer;
        friend class RenderViewport;

    public:

        EE_ENTITY_WORLD_SYSTEM( RenderWorldSystem );

        // Picking
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void UpdateViewportPickingData( RenderViewport* pViewport ) const;
        #endif

        // Outlines
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void SetOutlinedComponents( TArrayView<ComponentID> componentIDs );
        void ClearOutlinedComponents();
        #endif

    private:

        // Entity System
        //-------------------------------------------------------------------------

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override final;
        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;

        //-------------------------------------------------------------------------

        void UpdateDeviceResources();

        #if EE_DEVELOPMENT_TOOLS
        RHI::BufferHandle GetMeshInstanceRootOutlineBufferHandle() const;
        #endif

    private:

        // Internal renderer stuff
        //-------------------------------------------------------------------------

        void UpdateDirectionalLightShadows( Math::ViewVolume const& viewVolume );

    private:

        //-------------------------------------------------------------------------

        TaskSystem*                                                         m_pTaskSystem = nullptr;
        RenderSystem*                                                       m_pRenderSystem = nullptr;

        DeviceRenderWorld                                                   m_deviceRenderWorld;

        TIDVector<ComponentID, StaticMeshComponent const*>                  m_staticMeshComponents;
        TIDVector<ComponentID, SkeletalMeshComponent const*>                m_skeletalMeshComponents;
        TIDVector<ComponentID, PCGComponent const*>                         m_pcgComponents;
        TIDVector<ComponentID, DirectionalLightComponent*>                  m_directionalLightComponents;
        TIDVector<ComponentID, PointLightComponent*>                        m_pointLightComponents;
        TIDVector<ComponentID, SpotLightComponent*>                         m_spotLightComponents;

        TEntityMessageQueue<StaticMeshComponent>                            m_staticMeshComponentInstanceUpdateQueue;
        TEntityMessageQueue<SkeletalMeshComponent>                          m_skeletalMeshComponentInstanceUpdateQueue;

        #if EE_DEVELOPMENT_TOOLS
        DeviceResizeBuffer                                                  m_meshInstanceRootOutlineBuffer = {};
        TAlignedVector<uint64_t>                                            m_meshInstanceRootOutlineData;
        bool                                                                m_meshInstanceRootOutlineNeedUpdate = false;
        TVector<ComponentID>                                                m_outlinedComponents;
        #endif
    };
}
