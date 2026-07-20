#include "Engine/Physics/Physics.h"
#include "EngineTools/PropertyGrid/PropertyGridEditor.h"
#include "Engine/Physics/Ragdoll/PhysicsRagdoll_Definition.h"
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

        CollisionSettingsEditor( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance ) 
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            UpdateCollisionMaskWorkingData( m_value_imgui );
            m_isEditingRagdollShape = TryCast<RagdollShapeDefinition>( pTypeInstance ) != nullptr;
        }

    private:

        virtual void UpdatePropertyValue() override
        {
            auto pSettings = reinterpret_cast<CollisionSettings*>( m_pPropertyInstance );
            *pSettings = m_value_imgui;
            m_value_cached = m_value_imgui;

            UpdateCollisionMaskWorkingData( m_value_imgui );
        }

        virtual void ResetWorkingCopy() override
        {
            auto pSettings = reinterpret_cast<CollisionSettings*>( m_pPropertyInstance );
            m_value_imgui = *pSettings;
            m_value_cached = *pSettings;

            UpdateCollisionMaskWorkingData( m_value_imgui );
        }

        virtual void HandleExternalUpdate() override
        {
            auto pSettings = reinterpret_cast<CollisionSettings*>( m_pPropertyInstance );
            if ( *pSettings != m_value_cached )
            {
                ResetWorkingCopy();
            }
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            //-------------------------------------------------------------------------

            TypeSystem::EnumInfo const* pObjectCategoryEnumInfo = GetTypeRegistry()->GetEnumInfo<ObjectCategory>();
            EE_ASSERT( pObjectCategoryEnumInfo != nullptr );

            TypeSystem::EnumInfo const* pQueryCategoryEnumInfo = GetTypeRegistry()->GetEnumInfo<QueryCategory>();
            EE_ASSERT( pQueryCategoryEnumInfo != nullptr );

            //-------------------------------------------------------------------------
            // Object Categories
            //-------------------------------------------------------------------------

            if ( !m_isEditingRagdollShape )
            {
                ImGui::SeparatorText( "Collider Type" );

                char const* const pPreviewValue = pObjectCategoryEnumInfo->GetConstantLabel( (uint8_t) m_value_imgui.m_category ).c_str();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::BeginCombo( "##ColliderType", pPreviewValue ) )
                {
                    for ( auto const& constant : pObjectCategoryEnumInfo->m_constants )
                    {
                        if ( ImGui::MenuItem( constant.m_ID.c_str() ) )
                        {
                            m_value_imgui.m_category = (ObjectCategory) constant.m_value;
                            valueChanged = true;
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            //-------------------------------------------------------------------------
            // Response Settings
            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Collides With" );

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
            if ( ImGui::BeginTable( "FlagsTable", 2, ImGuiTableFlags_SizingFixedFit ) )
            {
                ImGui::TableSetupColumn( "Col0", ImGuiTableColumnFlags_WidthStretch, 0.5f );
                ImGui::TableSetupColumn( "Col1", ImGuiTableColumnFlags_WidthStretch, 0.5f );

                ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
                int32_t flagCount = 0;

                for ( auto const& constant : pObjectCategoryEnumInfo->m_constants )
                {
                    if ( ( flagCount % 2 ) == 0 )
                    {
                        ImGui::TableNextRow();
                    }

                    ImGui::TableNextColumn();
                    flagCount++;

                    //-------------------------------------------------------------------------

                    int64_t const flagValue = constant.m_value;
                    EE_ASSERT( flagValue >= 0 && flagValue < 32 );
                    if ( ImGui::Checkbox( constant.m_ID.c_str(), &m_workingData[flagValue] ) )
                    {
                        OnCollidesWithFlagsChanged();
                        valueChanged = true;
                    }
                }

                for ( auto const& constant : pQueryCategoryEnumInfo->m_constants )
                {
                    if ( ( flagCount % 2 ) == 0 )
                    {
                        ImGui::TableNextRow();
                    }

                    ImGui::TableNextColumn();
                    flagCount++;

                    //-------------------------------------------------------------------------

                    int64_t const flagValue = constant.m_value;
                    EE_ASSERT( flagValue >= 32 && flagValue < 64 );
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

            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        void UpdateCollisionMaskWorkingData( CollisionSettings const& settings )
        {
            TypeSystem::EnumInfo const* pObjectCategoryEnumInfo = GetTypeRegistry()->GetEnumInfo<ObjectCategory>();
            EE_ASSERT( pObjectCategoryEnumInfo != nullptr );

            TypeSystem::EnumInfo const* pQueryCategoryEnumInfo = GetTypeRegistry()->GetEnumInfo<QueryCategory>();
            EE_ASSERT( pQueryCategoryEnumInfo != nullptr );

            for ( auto const& constant : pObjectCategoryEnumInfo->m_constants )
            {
                m_workingData[constant.m_value] = settings.m_collisionMask & ( uint64_t( 1 ) << constant.m_value );
            }

            for ( auto const& constant : pQueryCategoryEnumInfo->m_constants )
            {
                m_workingData[constant.m_value] = settings.m_collisionMask & ( uint64_t( 1 ) << constant.m_value );
            }
        }

        void OnCollidesWithFlagsChanged()
        {
            m_value_imgui.m_collisionMask = 0;

            for ( uint8_t i = 0; i < 64; i++ )
            {
                if ( m_workingData[i] )
                {
                    m_value_imgui.m_collisionMask |= ( uint64_t( 1 ) << i );
                }
            }
        }

    private:

        bool                        m_workingData[64];
        CollisionSettings           m_value_imgui;
        CollisionSettings           m_value_cached;
        bool                        m_isEditingRagdollShape = false;
    };

    //-------------------------------------------------------------------------

    EE_PROPERTY_GRID_TYPE_EDITOR( CollisionSettingsEditor, Physics::CollisionSettings );
}