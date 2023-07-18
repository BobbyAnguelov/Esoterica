#include "Engine/Physics/PhysicsSettings.h"
#include "EngineTools/Core/PropertyGrid/PropertyGridEditor.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/EnumInfo.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class CollisionSettingsEditor final : public PG::PropertyEditor
    {
    public:

        using PropertyEditor::PropertyEditor;

        CollisionSettingsEditor( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, void* m_pPropertyInstance ) 
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            UpdateCollidesWithWorkingData( m_value_imgui );
        }

    private:

        virtual void UpdatePropertyValue() override
        {
            auto pSettings = reinterpret_cast<CollisionSettings*>( m_pPropertyInstance );
            *pSettings = m_value_imgui;
            m_value_cached = m_value_imgui;

            UpdateCollidesWithWorkingData( m_value_imgui );
        }

        virtual void ResetWorkingCopy() override
        {
            auto pSettings = reinterpret_cast<CollisionSettings*>( m_pPropertyInstance );
            m_value_imgui = *pSettings;
            m_value_cached = *pSettings;

            UpdateCollidesWithWorkingData( m_value_imgui );
        }

        virtual void HandleExternalUpdate() override
        {
            auto pSettings = reinterpret_cast<CollisionSettings*>( m_pPropertyInstance );
            if ( *pSettings != m_value_cached )
            {
                ResetWorkingCopy();
            }
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            //-------------------------------------------------------------------------

            TypeSystem::EnumInfo const* pColliderTypeEnumInfo = GetTypeRegistry()->GetEnumInfo<CollisionCategory>();
            EE_ASSERT( pColliderTypeEnumInfo != nullptr );

            TypeSystem::EnumInfo const* pQueryChannelEnumInfo = GetTypeRegistry()->GetEnumInfo<QueryChannel>();
            EE_ASSERT( pQueryChannelEnumInfo != nullptr );

            //-------------------------------------------------------------------------
            // Object Categories
            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Collider Type" );

            char const* const pPreviewValue = pColliderTypeEnumInfo->GetConstantLabel( (uint8_t) m_value_imgui.m_category ).c_str();
            ImGui::SetNextItemWidth( -1 );
            if ( ImGui::BeginCombo( "##ColliderType", pPreviewValue ) )
            {
                for ( auto const& constant : pColliderTypeEnumInfo->m_constants )
                {
                    if ( ImGui::MenuItem( constant.m_ID.c_str() ) )
                    {
                        m_value_imgui.m_category = (CollisionCategory) constant.m_value;
                        valueChanged = true;
                    }
                }
                ImGui::EndCombo();
            }

            //-------------------------------------------------------------------------
            // Response Settings
            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Collides With" );

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
            if ( ImGui::BeginTable( "FlagsTable", 2, ImGuiTableFlags_SizingFixedFit ) )
            {
                ImGui::TableSetupColumn( "Col0", ImGuiTableColumnFlags_WidthStretch, 0.5f );
                ImGui::TableSetupColumn( "Col1", ImGuiTableColumnFlags_WidthStretch, 0.5f );

                ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
                int32_t flagCount = 0;

                for ( auto const& constant : pColliderTypeEnumInfo->m_constants )
                {
                    if ( ( flagCount % 2 ) == 0 )
                    {
                        ImGui::TableNextRow();
                    }

                    ImGui::TableNextColumn();
                    flagCount++;

                    //-------------------------------------------------------------------------

                    int64_t const flagValue = constant.m_value;
                    EE_ASSERT( flagValue >= 0 && flagValue <= 31 );
                    if ( ImGui::Checkbox( constant.m_ID.c_str(), &m_workingData[flagValue] ) )
                    {
                        OnCollidesWithFlagsChanged();
                        valueChanged = true;
                    }
                }

                for ( auto const& constant : pQueryChannelEnumInfo->m_constants )
                {
                    if ( ( flagCount % 2 ) == 0 )
                    {
                        ImGui::TableNextRow();
                    }

                    ImGui::TableNextColumn();
                    flagCount++;

                    //-------------------------------------------------------------------------

                    int64_t const flagValue = constant.m_value;
                    EE_ASSERT( flagValue >= 0 && flagValue <= 31 );
                    if ( ImGui::Checkbox( constant.m_ID.c_str(), &m_workingData[flagValue] ) )
                    {
                        OnCollidesWithFlagsChanged();
                        valueChanged = true;
                    }
                }

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();

            //-------------------------------------------------------------------------

            return valueChanged;
        }

        void UpdateCollidesWithWorkingData( CollisionSettings const& settings )
        {
            TypeSystem::EnumInfo const* pColliderTypeEnumInfo = GetTypeRegistry()->GetEnumInfo<CollisionCategory>();
            EE_ASSERT( pColliderTypeEnumInfo != nullptr );

            TypeSystem::EnumInfo const* pQueryChannelEnumInfo = GetTypeRegistry()->GetEnumInfo<QueryChannel>();
            EE_ASSERT( pQueryChannelEnumInfo != nullptr );

            for ( auto const& constant : pColliderTypeEnumInfo->m_constants )
            {
                m_workingData[constant.m_value] = settings.m_collidesWithMask & ( 1u << constant.m_value );
            }

            for ( auto const& constant : pQueryChannelEnumInfo->m_constants )
            {
                m_workingData[constant.m_value] = settings.m_collidesWithMask & ( 1u << constant.m_value );
            }
        }

        void OnCollidesWithFlagsChanged()
        {
            m_value_imgui.m_collidesWithMask = 0;

            for ( uint8_t i = 0; i < 16; i++ )
            {
                if ( m_workingData[i] )
                {
                    m_value_imgui.m_collidesWithMask |= ( 1u << i );
                }
            }
        }

    private:

        bool                    m_workingData[16];
        CollisionSettings            m_value_imgui;
        CollisionSettings            m_value_cached;
    };

    //-------------------------------------------------------------------------

    EE_PROPERTY_GRID_TYPE_EDITOR( CollisionSettingsEditorFactory, Physics::CollisionSettings, CollisionSettingsEditor );
}