#pragma once

#include "Engine/Entity/EntityWorldSystem.h"
#include "Engine/Entity/EntityWorldSystemSignal.h"

#include "Engine/Render/Device/DeviceRenderWorld.h"
#include "Engine/Render/RenderMaterialShaderClusterCapacity.h"
#include "Engine/Viewport/ViewportPicking.h"

#include "Base/Resource/ResourcePtr.h"
#include "Base/Render/RHI.h"
#include "Base/Systems.h"
#include "Base/Types/IDVector.h"

//-----------------------------------------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------------------------------------

    class EE_ENGINE_API RenderWorldSystem final : public EntityWorldSystem
    {
        friend class ForwardShadingRenderer;
        friend class RenderDebugView;

    public:

        EE_ENTITY_WORLD_SYSTEM( RenderWorldSystem );

        void UpdateViewportPickingData( RenderViewport* pViewport ) const;

    private:

        // Entity System
        //---------------------------------------------------------------------------------------------------

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override final;
        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;

        //-------------------------------------------------------------------------

        void AddMeshClusters( Mesh const* pMeshResource, TArrayView<TResourcePtr<Material> const> materialOverrides, TBitFlags<ViewLayer> viewLayers );
        void RemoveMeshClusters( Mesh const* pMeshResource, TArrayView<TResourcePtr<Material> const> materialOverrides, TBitFlags<ViewLayer> viewLayers );

        //-------------------------------------------------------------------------

        void UpdateDeviceResources();

        RHI::TextureHandle GetRadianceTextureHandle() const;
        float GetRadianceTextureMipLevels() const;
        RHI::TextureHandle GetIrradianceTextureHandle() const;

    private:

        //---------------------------------------------------------------------------------------------------

        TaskSystem*                                                         m_pTaskSystem = nullptr;
        RenderSystem*                                                       m_pRenderSystem = nullptr;

        DeviceRenderWorld                                                   m_deviceRenderWorld;

        TIDVector<ComponentID, StaticMeshComponent const*>                  m_staticMeshComponents;
        TIDVector<ComponentID, SkeletalMeshComponent const*>                m_skeletalMeshComponents;
        TIDVector<ComponentID, PCGComponent const*>                         m_pcgComponents;
        TIDVector<ComponentID, DirectionalLightComponent const*>            m_directionalLightComponents;
        TIDVector<ComponentID, PointLightComponent const*>                  m_pointLightComponents;
        TIDVector<ComponentID, SpotLightComponent const*>                   m_spotLightComponents;

        TEntityMessageQueue<StaticMeshComponent>                            m_staticMeshComponentInstanceUpdateQueue;
        TEntityMessageQueue<SkeletalMeshComponent>                          m_skeletalMeshComponentInstanceUpdateQueue;

        uint32_t                                                            m_numShadowCastingDirectionalLights = 0;

        MaterialShaderClusterCapacity                                       m_materialShaderClusterCapacity;

        bool                                                                m_needUpdateGlobalEnvironmentMap = true;
        RHI::Texture*                                                       m_pRadianceTexture = nullptr;
        RHI::Texture*                                                       m_pIrradianceTexture = nullptr;
    };
}
