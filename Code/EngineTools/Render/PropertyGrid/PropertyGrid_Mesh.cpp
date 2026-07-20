#include "Engine/Physics/Physics.h"
#include "Engine/Render/Components/Component_RenderMesh.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class MeshComponentEditingRules : public PG::TTypeEditingRules<MeshComponent>
    {
        using PG::TTypeEditingRules<MeshComponent>::TTypeEditingRules;

        virtual InlineString GetPropertyNameOverride( StringID const& propertyID, int32_t arrayElementIdx )
        {
            static StringID const s_materialOverridesPropertyID = StringID( "m_materialOverrides" );
            if ( propertyID == s_materialOverridesPropertyID )
            {
                if ( m_pTypeInstance->IsInitialized() && m_pTypeInstance->HasMeshResourceSet() )
                {
                    StringID const materialMappingID = m_pTypeInstance->GetSubmeshID( arrayElementIdx );
                    return materialMappingID.IsValid() ? materialMappingID.c_str() : InlineString();
                }
            }

            return InlineString();
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( MeshComponent, MeshComponentEditingRules );
    EE_PROPERTY_GRID_EDITING_RULES( StaticMeshComponent, MeshComponentEditingRules );
    EE_PROPERTY_GRID_EDITING_RULES( SkeletalMeshComponent, MeshComponentEditingRules );

    //-------------------------------------------------------------------------

    class MeshMaterialMappingEditingRules : public PG::TTypeEditingRules<MeshMaterialMapping>
    {
        using PG::TTypeEditingRules<MeshMaterialMapping>::TTypeEditingRules;

        virtual InlineString GetPropertyNameOverride( StringID const& propertyID, int32_t arrayElementIdx )
        {
            return m_pTypeInstance->m_mappingID.IsValid() ? m_pTypeInstance->m_mappingID.c_str() : "";
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( MeshMaterialMapping, MeshMaterialMappingEditingRules );
}