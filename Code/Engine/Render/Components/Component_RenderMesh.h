#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderViewLayer.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/RenderMaterial.h"
#include "Engine/Entity/EntitySpatialComponent.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Material;
    class Mesh;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API MeshComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( MeshComponent );

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

        virtual void OnRenderInstanceDataUpdated() = 0;

    protected:

        uint32_t ComputeInstanceDataSizeInBytes( Mesh const* pMeshResource ) const;
        void WriteInstanceData( Mesh const* pMeshResource, HandleAllocator<uint32_t>::Handle meshInstanceHandle, HandleAllocator<uint32_t>::Handle bonesHandle, TArrayView<uint32_t> bufferData_WriteCombined ) const;

        void ValidateAndFixSubmeshSettings();

    protected:

        EE_REFLECT();
        bool                                    m_componentHidden = false;

        EE_REFLECT( Category = "Mesh" );
        TBitFlags<ViewLayer>                    m_viewLayers = TBitFlags<ViewLayer>( ViewLayer::ShadowMap, ViewLayer::ForwardShading );

        EE_REFLECT( Category = "Mesh" );
        int32_t                                 m_forcedMinLOD = -1;

        EE_REFLECT( Category = "Mesh" );
        int32_t                                 m_forcedLOD = -1;

        EE_REFLECT( Category = "Submeshes" );
        SubmeshSettings                         m_submeshSettings;
    };
}