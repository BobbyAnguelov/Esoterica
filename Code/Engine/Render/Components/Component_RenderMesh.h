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
        bool IsSubmeshVisible( int32_t submeshIdx ) const;

        // Set all submesh visibility
        void SetAllSubmeshVisibility( bool isVisible );

        // Set an individual instances visibility
        void SetSubmeshVisibility( int32_t submeshIdx, bool isVisible );

        // Set visibility for several sections
        void SetSubmeshVisibility( TVector<TPair<int32_t, bool>> const& visibility ) { SetSubmeshVisibility( visibility.data(), visibility.size() ); }

        // Set visibility for several sections
        template<size_t N>
        void SetSubmeshVisibility( TInlineVector<TPair<int32_t, bool>, N> const& visibility ) { SetSubmeshVisibility( visibility.data(), visibility.size() ); }

        // Materials
        //-------------------------------------------------------------------------

        // Do we have a material override set for a submesh
        bool IsMaterialOverriden( int32_t submeshIdx ) const;

        // Get the material override for a given submesh
        Material const* GetMaterialOverride( int32_t submeshIdx ) const;

        // Get the material override resourceID for a submesh - returns invalid resource ID if not set
        ResourceID GetMaterialOverrideResourceID( int32_t submeshIdx ) const;

        // Set a material override for a submesh
        void SetMaterialOverride( int32_t submeshIdx, ResourceID const& materialResourceID );

        // Clear all material overrides
        void ClearMaterialOverrides();

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

        void SetSubmeshVisibility( TPair<int32_t, bool> const* pData, size_t size );

    protected:

        EE_REFLECT();
        bool                                    m_componentHidden = false;

        EE_REFLECT();
        TVector<bool>                           m_submeshesHidden;

        EE_REFLECT( ShowAsStaticArray );
        TVector<TResourcePtr<Material>>         m_materialOverrides;

        EE_REFLECT();
        TBitFlags<ViewLayer>                    m_viewLayers = TBitFlags<ViewLayer>( ViewLayer::ShadowMap, ViewLayer::ForwardShading );

        EE_REFLECT();
        int32_t                                 m_forcedMinLOD = -1;

        EE_REFLECT();
        int32_t                                 m_forcedLOD = -1;
    };
}