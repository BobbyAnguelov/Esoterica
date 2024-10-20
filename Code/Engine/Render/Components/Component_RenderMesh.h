#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Engine/Entity/EntitySpatialComponent.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API MeshComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( MeshComponent );

    public:

        inline MeshComponent() = default;
        inline MeshComponent( StringID name ) : SpatialEntityComponent( name ) {}

        // Does this component have a valid mesh set
        virtual bool HasMeshResourceSet() const = 0;

        // Visibility
        //-------------------------------------------------------------------------

        // Is this mesh visible
        inline bool IsVisible() const { return m_isVisible; }

        // Set the entire mesh visibility
        inline void SetVisible( bool isVisible ) { m_isVisible = isVisible; }

        // Set section visibility
        inline void SetSectionVisibility( int32_t sectionIdx, bool isVisible )
        {
            EE_ASSERT( sectionIdx >= 0 && sectionIdx < 64 );
            if ( isVisible )
            {
                m_sectionVisibilityMask |=  1ull << sectionIdx;
            }
            else
            {
                m_sectionVisibilityMask &= ~( 1ull << sectionIdx );
            }
        }

        // Is a given section visible?
        EE_FORCE_INLINE bool IsSectionVisible( int32_t sectionIdx ) const
        {
            EE_ASSERT( sectionIdx >= 0 && sectionIdx < 64 );
            return ( m_sectionVisibilityMask & ( 1ull << sectionIdx ) ) != 0;
        }

        // Get the section visibility mask
        EE_FORCE_INLINE uint64_t GetSectionVisibilityMask() const { return m_sectionVisibilityMask; }

        // Materials
        //-------------------------------------------------------------------------

        inline int32_t GetNumRequiredMaterials() const { return (int32_t) GetDefaultMaterials().size(); }
        inline TVector<Material const*> const& GetMaterials() const { return m_materials; }
        void SetMaterialOverride( int32_t materialIdx, ResourceID materialResourceID );

    protected:

        virtual void Initialize() override;
        virtual void Shutdown() override;

        // Get the default materials for the set mesh
        virtual TVector<TResourcePtr<Material>> const& GetDefaultMaterials() const = 0;

    private:

        void UpdateMaterialCache();

    private:

        // Any user material overrides
        EE_REFLECT();
        TVector<TResourcePtr<Material>>                         m_materialOverrides;

        // The final set of materials to use for this component
        TVector<Material const*>                                m_materials;

        EE_REFLECT( ReadOnly );
        uint64_t                                                m_sectionVisibilityMask = 0xFFFFFFFF;

        // Should this component be rendered
        bool                                                    m_isVisible = true;
    };
}