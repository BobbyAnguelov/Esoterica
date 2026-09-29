#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderViewLayer.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/RenderMaterial.h"
#include "Engine/Render/RenderProxies.h"
#include "Base/Render/RHI.h"
#include "Engine/Entity/EntitySpatialComponent.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Material;
    class Mesh;
    class DeviceRenderWorld;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API MeshComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( MeshComponent );

        friend class RenderWorldSystem;

    public:

        struct EE_ENGINE_API MaterialOverride : public IReflectedType
        {
            EE_REFLECT_TYPE( MaterialOverride );

            MaterialOverride() = default;

            MaterialOverride( int16_t submeshIdx, ResourceID const& materialID = ResourceID() )
                : m_submeshIdx( submeshIdx )
                , m_material( materialID )
            {}

            inline bool operator==( MaterialOverride const& rhs ) const
            {
                return m_submeshIdx == rhs.m_submeshIdx && m_material == rhs.m_material;
            }

        public:

            EE_REFLECT();
            int16_t                                 m_submeshIdx = InvalidIndex;

            EE_REFLECT();
            TResourcePtr<Material>                  m_material;
        };

        //-------------------------------------------------------------------------

        struct EE_ENGINE_API SubmeshSettings : public IReflectedType
        {
            EE_REFLECT_TYPE( SubmeshSettings );

            inline void Clear() { m_hiddenSubmeshes.clear(); m_materialOverrides.clear(); }

            MaterialOverride* GetMaterialOverride( int16_t submeshIdx );
            inline MaterialOverride const* GetMaterialOverride( int16_t submeshIdx ) const { return const_cast<SubmeshSettings*>( this )->GetMaterialOverride( submeshIdx ); }

            bool operator==( SubmeshSettings const& rhs ) const;

        public:

            EE_REFLECT();
            TVector<int16_t>                        m_hiddenSubmeshes;

            EE_REFLECT( ShowAsStaticArray );
            TVector<MaterialOverride>               m_materialOverrides;
        };

        //-------------------------------------------------------------------------

        struct SubmeshToMeshInstance                                                    // TODO: Not everything in this struct is needed all the time, we can split it potentially
        {
            RHI::Buffer*                m_pMeshBuffer = nullptr;
            uint32_t                    m_submeshIndex = ~0U;                           // This can potentially be removed
            int32_t                     m_shaderIndex = -1;
            uint32_t                    m_numClusters = 0;
            uint32_t                    m_proxyIndex = ~0U;
            uint32_t                    m_instanceIndex = ~0U;
            uint32_t                    m_clusterToInstanceOffset = ~0U;
            uint32_t                    m_shaderParametersOffsetIn32ByteBlocks = 0;
            uint8_t                     m_lodMask = 0;
            bool                        m_instanceHidden = false;
        };

    public:

        inline MeshComponent() = default;
        inline MeshComponent( StringID name ) : SpatialEntityComponent( name ) {}

        virtual void Initialize() override;

        void SetViewLayers( TBitFlags<ViewLayer> viewLayers );

        // Mesh Info
        //-------------------------------------------------------------------------

        virtual bool HasMeshResourceSet() const = 0;
        virtual Mesh const* GetMeshResource() const = 0;
        inline bool IsMeshLoaded() const { return GetMeshResource() != nullptr; }

        // Submesh
        //-------------------------------------------------------------------------

        int32_t GetNumSubmeshes() const;

        StringID GetSubmeshID( int32_t submeshIdx ) const;

        // Visibility
        //-------------------------------------------------------------------------

        // Is this component visible
        inline bool IsVisible() const { return !m_componentHidden; }

        // Set component visibility
        void SetVisible( bool visible );

        // Is a given submesh visible?
        bool IsSubmeshVisible( int16_t submeshIdx ) const { return !VectorContains( m_submeshSettings.m_hiddenSubmeshes, submeshIdx ); }

        // Set an individual instances visibility
        void SetSubmeshVisibility( int16_t submeshIdx, bool isVisible );

        // Bulk set submesh visibility
        void SetSubmeshVisibility( TVector<int16_t> const& hiddenSubmeshes );

        // Materials
        //-------------------------------------------------------------------------

        // Do we have a material override set for a submesh
        bool IsMaterialOverridden( int16_t submeshIdx ) const;

        // Get the material override for a given submesh
        Material const* GetMaterialOverride( int16_t submeshIdx ) const;

        // Get the material override resourceID for a submesh - returns invalid resource ID if not set
        ResourceID GetMaterialOverrideResourceID( int16_t submeshIdx ) const;

        // Set a material override for a submesh
        void SetMaterialOverride( int16_t submeshIdx, ResourceID const& materialResourceID );

        // Clear all material overrides
        void ClearMaterialOverrides();

        // Get the final set of material for the loaded mesh
        TInlineVector<Material const*, 50> GetResolvedMaterials() const;

        // LOD
        //-------------------------------------------------------------------------

        inline bool HasForcedMinLOD() const { return m_forcedMinLOD >= 0; }
        void SetForcedMinLOD( int32_t forceMinLOD );
        int32_t GetForcedMinLOD() const;

        inline bool HasForcedLOD() const { return m_forcedLOD >= 0; }
        void SetForcedLOD( int32_t forceLOD );
        int32_t GetForcedLOD() const;

    protected:

        //-------------------------------------------------------------------------

        void ValidateAndFixSubmeshSettings();

        virtual void OnRenderInstanceDataUpdated() = 0;

        // Internal renderer functions
        //-------------------------------------------------------------------------

        void AllocateMeshInstanceProxies( DeviceRenderWorld* pDeviceRenderWorld );
        void QueueMeshInstanceInitialize( DeviceRenderWorld* pDeviceRenderWorld, Material const* pPlaceholderMaterial );

        void WriteInstanceData( Mesh const* pMeshResource, uint32_t boneOffset, TArrayView<uint32_t> bufferData_WriteCombined ) const;
        void WriteMeshInstanceRootTransform();
        void WriteMeshInstanceLocalTransforms();

        void ResolveSubmeshMaterials( Material const* pPlaceholderMaterial );
        void ResolveSubmeshProxyData( uint32_t numShaderPools );

        void UpdateSubmeshVisibility();

        void ValidateSubmeshInstanceData( Material const* pPlaceholderMaterial ) const;

        //-------------------------------------------------------------------------

        EE_REFLECT();
        bool                                            m_componentHidden = false;

        EE_REFLECT( Category = "Mesh" );
        TBitFlags<ViewLayer>                            m_viewLayers = TBitFlags<ViewLayer>( ViewLayer::ShadowMap, ViewLayer::ForwardShading );

        EE_REFLECT( Category = "Mesh" );
        int32_t                                         m_forcedMinLOD = -1;

        EE_REFLECT( Category = "Mesh" );
        int32_t                                         m_forcedLOD = -1;

        EE_REFLECT( Category = "Submeshes" );
        SubmeshSettings                                 m_submeshSettings;

        // Internal renderer data
        //-------------------------------------------------------------------------

        TVector<SubmeshToMeshInstance>                  m_submeshToMeshInstance = {};

        MeshInstanceRootProxy                           m_meshInstanceRootProxy = {};
        TVector<TPair<uint32_t, MeshInstanceProxy>>     m_meshInstanceProxies = {};
    };
}