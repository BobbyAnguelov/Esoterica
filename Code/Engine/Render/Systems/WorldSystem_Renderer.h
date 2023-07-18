#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Math/AABBTree.h"
#include "Base/Types/Event.h"
#include "Base/Systems.h"
#include "Base/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class SkeletalMeshComponent;
    class DirectionalLightComponent;
    class PointLightComponent;
    class SpotLightComponent;
    class GlobalEnvironmentMapComponent;
    class LocalEnvironmentMapComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RendererWorldSystem final : public EntityWorldSystem
    {
        friend class WorldRenderer;
        friend class RenderDebugView;

    public:

        EE_ENTITY_WORLD_SYSTEM( RendererWorldSystem, RequiresUpdate( UpdateStage::FrameEnd ), RequiresUpdate( UpdateStage::Paused ) );

        #if EE_DEVELOPMENT_TOOLS
        enum class VisualizationMode : int8_t
        {
            Lighting            = 0,
            Albedo              = 1,
            Normals             = 2,
            Metalness           = 3,
            Roughness           = 4,
            AmbientOcclusion    = 5,

            BitShift            = 32 - 3,
        };
        #endif

    private:

        // Track all instances of a given mesh together - to limit the number of vertex buffer changes
        struct SkeletalMeshGroup
        {
            SkeletalMeshGroup( SkeletalMesh const* pInMesh ) : m_pMesh( pInMesh ) { EE_ASSERT( pInMesh != nullptr ); }

            inline uint32_t GetID() const { return m_pMesh->GetResourceID().GetPathID(); }

        public:

            SkeletalMesh const*                                 m_pMesh = nullptr;
            TVector<SkeletalMeshComponent*>                     m_components;
        };

    public:

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void SetVisualizationMode( VisualizationMode mode ) { m_visualizationMode = mode; }
        VisualizationMode GetVisualizationMode() { return m_visualizationMode; }
        #endif

    private:

        // Entity System
        //-------------------------------------------------------------------------

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override final;
        virtual void ShutdownSystem() override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;

        // Static Meshes
        //-------------------------------------------------------------------------

        void RegisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent );
        void UnregisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent );
        void OnStaticMeshMobilityUpdated( StaticMeshComponent* pComponent );
        void OnStaticMobilityComponentTransformUpdated( StaticMeshComponent* pComponent );

        // Skeletal Meshes
        //-------------------------------------------------------------------------

        void RegisterSkeletalMeshComponent( Entity const* pEntity, SkeletalMeshComponent* pMeshComponent );
        void UnregisterSkeletalMeshComponent( Entity const* pEntity, SkeletalMeshComponent* pMeshComponent );

    private:

        // Static meshes
        TIDVector<ComponentID, StaticMeshComponent*>                    m_registeredStaticMeshComponents;
        TIDVector<ComponentID, StaticMeshComponent*>                    m_staticStaticMeshComponents;
        TIDVector<ComponentID, StaticMeshComponent*>                    m_dynamicStaticMeshComponents;
        TVector<StaticMeshComponent const*>                             m_visibleStaticMeshComponents;
        EventBindingID                                                  m_staticMeshMobilityChangedEventBinding;
        EventBindingID                                                  m_staticMeshStaticTransformUpdatedEventBinding;
        Threading::Mutex                                                m_mobilityUpdateListLock;               // Mobility switches can occur on any thread so the list needs to be threadsafe. We use a simple lock for now since we dont expect too many switches
        TVector<StaticMeshComponent*>                                   m_mobilityUpdateList;                   // A list of all components that switched mobility during this frame, will results in an update of the various spatial data structures next frame
        TVector<StaticMeshComponent*>                                   m_staticMobilityTransformUpdateList;    // A list of all static mobility components that have moved during this frame, will results in an update of the various spatial data structures next frame
        Math::AABBTree                                                  m_staticMobilityTree;

        // Skeletal meshes
        TIDVector<ComponentID, SkeletalMeshComponent*>                  m_registeredSkeletalMeshComponents;
        TIDVector<uint32_t, SkeletalMeshGroup>                          m_skeletalMeshGroups;
        TVector<SkeletalMeshComponent const*>                           m_visibleSkeletalMeshComponents;

        // Lights
        TIDVector<ComponentID, DirectionalLightComponent*>              m_registeredDirectionLightComponents;
        TIDVector<ComponentID, PointLightComponent*>                    m_registeredPointLightComponents;
        TIDVector<ComponentID, SpotLightComponent*>                     m_registeredSpotLightComponents;
        TIDVector<ComponentID, LocalEnvironmentMapComponent*>           m_registeredLocalEnvironmentMaps;
        TIDVector<ComponentID, GlobalEnvironmentMapComponent*>          m_registeredGlobalEnvironmentMaps;

        #if EE_DEVELOPMENT_TOOLS
        VisualizationMode                                               m_visualizationMode = VisualizationMode::Lighting;
        bool                                                            m_showStaticMeshBounds = false;
        bool                                                            m_showSkeletalMeshBounds = false;
        bool                                                            m_showSkeletalMeshBones = false;
        bool                                                            m_showSkeletalMeshBindPoses = false;
        #endif
    };
}