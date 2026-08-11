#include "Engine/Render/Components/Component_RenderMesh.h"
#include "EngineTools/PropertyGrid/PropertyGridEditor.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/EnumInfo.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "EngineTools/Widgets/Pickers/ResourcePickers.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class SubmeshSettingsEditor final : public PG::PropertyEditor
    {
    public:

        using PropertyEditor::PropertyEditor;

        SubmeshSettingsEditor( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {}

        ~SubmeshSettingsEditor()
        {
            for ( auto pPicker : m_pickers )
            {
                EE::Delete( pPicker );
            }
        }

    private:

        virtual void UpdatePropertyValue() override
        {
            auto pSettings = reinterpret_cast<MeshComponent::SubmeshSettings*>( m_pPropertyInstance );
            *pSettings = m_value_imgui;
            m_value_cached = m_value_imgui;
        }

        virtual void ResetWorkingCopy() override
        {
            auto pSettings = reinterpret_cast<MeshComponent::SubmeshSettings*>( m_pPropertyInstance );

            m_value_imgui.Clear();
            m_value_cached.Clear();

            m_value_imgui.m_hiddenSubmeshes = pSettings->m_hiddenSubmeshes;
            m_value_cached.m_hiddenSubmeshes = pSettings->m_hiddenSubmeshes;

            for ( auto const& materialOverride : pSettings->m_materialOverrides )
            {
                m_value_imgui.m_materialOverrides.emplace_back( materialOverride.m_submeshIdx, materialOverride.m_material.GetResourceID() );
                m_value_cached.m_materialOverrides.emplace_back( materialOverride.m_submeshIdx, materialOverride.m_material.GetResourceID() );
            }
        }

        virtual void HandleExternalUpdate() override
        {
            auto pSettings = reinterpret_cast<MeshComponent::SubmeshSettings*>( m_pPropertyInstance );
            if ( *pSettings != m_value_cached )
            {
                ResetWorkingCopy();
            }
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            auto pMeshComponent = Cast<MeshComponent>( m_pTypeInstance );
            Mesh const* pMesh = pMeshComponent->GetMeshResource();
            if ( pMesh == nullptr )
            {
                ImGui::Text( "No Mesh Loaded" );
                return Result::None;
            }

            // Set up editor data
            //-------------------------------------------------------------------------

            int32_t const numSubmeshes = pMesh->GetNumSubmeshes();

            m_pickers.resize( Math::Max( m_pickers.size(), size_t( numSubmeshes ) ), nullptr ); // Never shrink
            m_isSubmeshVisible.resize( numSubmeshes, true );

            for ( int32_t i = 0; i < numSubmeshes; i++ )
            {
                if ( m_pickers[i] == nullptr )
                {
                    m_pickers[i] = EE::New<ResourcePicker>( *m_context.m_pToolsContext, Material::GetStaticResourceTypeID() );
                }

                m_pickers[i]->Clear();
                m_isSubmeshVisible[i] = true;
            }

            // Fill all resource IDs (and remove any invalid ones)
            for ( int32_t i = int32_t( m_value_imgui.m_materialOverrides.size() ) - 1; i >= 0; i-- )
            {
                int16_t const submeshIdx = m_value_imgui.m_materialOverrides[i].m_submeshIdx;

                if ( submeshIdx >= 0 && submeshIdx < numSubmeshes )
                {
                    m_pickers[submeshIdx]->SetResourceID( m_value_imgui.m_materialOverrides[i].m_material.GetResourceID() );
                }
                else // Remove invalid entry
                {
                    valueChanged = true;
                    m_value_imgui.m_materialOverrides.erase_unsorted( m_value_imgui.m_materialOverrides.begin() + i );
                }
            }

            // Fill all visibility state
            for ( int32_t i = int32_t( m_value_imgui.m_hiddenSubmeshes.size() ) - 1; i >= 0; i-- )
            {
                if ( m_value_imgui.m_hiddenSubmeshes[i] >= 0 && m_value_imgui.m_hiddenSubmeshes[i] < numSubmeshes )
                {
                    m_isSubmeshVisible[m_value_imgui.m_hiddenSubmeshes[i]] = false;
                }
                else // Remove invalid entry
                {
                    valueChanged = true;
                    m_value_imgui.m_hiddenSubmeshes.erase_unsorted( m_value_imgui.m_hiddenSubmeshes.begin() + i );
                }
            }

            // Draw editors
            //-------------------------------------------------------------------------

            for ( int16_t i = 0; i < numSubmeshes; i++ )
            {
                if ( i != 0 )
                {
                    ImGui::Separator();
                }

                ImGui::PushID( i );
                {
                    if ( ImGui::Checkbox( "##hidden", &m_isSubmeshVisible[i] ) )
                    {
                        UpdateVisibility( i );
                        valueChanged = true;
                    }
                    ImGuiX::ItemTooltip( "Is this sub-mesh visible?" );

                    ImGui::SameLine();
                    ImGui::Text( pMesh->GetSubmesh( i ).m_ID.c_str() );
                    if ( m_pickers[i]->UpdateAndDraw() )
                    {
                        UpdateMaterialOverride( i, m_pickers[i]->GetResourceID() );
                        valueChanged = true;
                    }
                }
                ImGui::PopID();
            }

            //-------------------------------------------------------------------------

            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        void UpdateVisibility( int16_t submeshIdx )
        {
            if ( m_isSubmeshVisible[submeshIdx] )
            {
                m_value_imgui.m_hiddenSubmeshes.erase_first( submeshIdx );
            }
            else
            {
                VectorEmplaceBackUnique( m_value_imgui.m_hiddenSubmeshes, submeshIdx );
            }
        }

        void UpdateMaterialOverride( int16_t submeshIdx, ResourceID const& materialResourceID )
        {
            int32_t const numOverrides = (int32_t) m_value_imgui.m_materialOverrides.size();
            for ( int32_t i = 0; i < numOverrides; i++ )
            {
                if ( m_value_imgui.m_materialOverrides[i].m_submeshIdx == submeshIdx )
                {
                    if ( materialResourceID.IsValid() )
                    {
                        m_value_imgui.m_materialOverrides[i].m_material = materialResourceID;
                    }
                    else // Remove override
                    {
                        m_value_imgui.m_materialOverrides.erase( m_value_imgui.m_materialOverrides.begin() + i );
                    }

                    return;
                }
            }

            //-------------------------------------------------------------------------

            m_value_imgui.m_materialOverrides.emplace_back( submeshIdx, materialResourceID );
        }

    private:

        TVector<ResourcePicker*>                        m_pickers;
        TVector<bool>                                   m_isSubmeshVisible;
        MeshComponent::SubmeshSettings                  m_value_imgui;
        MeshComponent::SubmeshSettings                  m_value_cached;
    };

    //-------------------------------------------------------------------------

    EE_PROPERTY_GRID_TYPE_EDITOR( SubmeshSettingsEditor, MeshComponent::SubmeshSettings );
}