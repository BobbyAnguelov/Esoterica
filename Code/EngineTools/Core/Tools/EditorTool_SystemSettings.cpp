#include "EditorTool_SystemSettings.h"
#include "Engine/UpdateContext.h"
#include "Base/Settings/Settings.h"
#include "Base/Settings/SettingsRegistry.h"

//-------------------------------------------------------------------------

namespace EE
{
    SystemSettingsEditorTool::SystemSettingsEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "System Settings" )
    {}

    void SystemSettingsEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );

        EE_ASSERT( m_pSettingsRegistry == nullptr );
        m_pSettingsRegistry = context.GetSystem<SettingsRegistry>();

        m_scratchBuffers.reserve( 100 );
        for ( int32_t i = 0; i < 100; i++ )
        {
            m_scratchBuffers.emplace_back( EE::New<ImGuiX::TextBuffer>() );
        }

        CreateToolWindow( "Log", [this] ( UpdateContext const& context, bool isFocused ) { DrawSettingsWindow( context, isFocused ); } );
    }

    void SystemSettingsEditorTool::Shutdown( UpdateContext const& context )
    {
        m_pSettingsRegistry = nullptr;

        for ( auto pBuffer : m_scratchBuffers )
        {
            EE::Delete( pBuffer );
        }

        m_scratchBuffers.clear();
        EditorTool::Shutdown( context );
    }

    void SystemSettingsEditorTool::DrawSettingsWindow( UpdateContext const& context, bool isFocused )
    {
        ResetScratchBufferFreeIndex();

        //-------------------------------------------------------------------------

        m_globalSettingsFilterWidget.UpdateAndDraw();

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "Settings", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Reset", ImGuiTableColumnFlags_WidthFixed, ImGuiX::CalculateButtonWidth( EE_ICON_UNDO_VARIANT ) );

            for ( Settings* pSettings : m_pSettingsRegistry->m_settings )
            {
                auto const& rootCategory = pSettings->m_categorizedConsoleEditableProperties.GetRootCategory();
                for ( auto const& childCategory : rootCategory.m_childCategories )
                {
                    DrawSettingsCategory( pSettings, childCategory );
                }
            }

            ImGui::EndTable();
        }
    }

    ImGuiX::TextBuffer* SystemSettingsEditorTool::GetScratchBuffer()
    {
        // Grow buffers
        //-------------------------------------------------------------------------

        if ( m_scratchBufferFreeIdx == m_scratchBuffers.size() )
        {
            for ( int32_t i = 0; i < 100; i++ )
            {
                m_scratchBuffers.emplace_back( EE::New<ImGuiX::TextBuffer>() );
            }
        }

        m_scratchBufferFreeIdx++;
        return m_scratchBuffers[m_scratchBufferFreeIdx - 1];
    }

    void SystemSettingsEditorTool::DrawSettingsCategory( Settings* pSettings, Category<TypeSystem::PropertyInfo const*> const& category )
    {
        auto FilterFunc = [this] ( CategoryItem<TypeSystem::PropertyInfo const*> const& item )
        {
            return m_globalSettingsFilterWidget.MatchesFilter( item.m_fullPath );
        };

        InlineString const categoryLabel( category.m_name.empty() ? pSettings->GetSectionName() : category.m_name.c_str() );
        if ( !category.HasItemsThatMatchFilter( FilterFunc ) )
        {
            return;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        if ( ImGui::TreeNodeEx( category.m_name.c_str(), ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull ) )
        {
            for ( auto const& childCategory : category.m_childCategories )
            {
                DrawSettingsCategory( pSettings, childCategory );
            }

            for ( auto const& item : category.m_items )
            {
                if ( m_globalSettingsFilterWidget.MatchesFilter( item.m_fullPath ) )
                {
                    DrawSettingsRow( pSettings, item.m_data );
                }
            }

            ImGui::TreePop();
        }
    }

    void SystemSettingsEditorTool::DrawSettingsRow( Settings* pSettings, TypeSystem::PropertyInfo const* pPropertyInfo )
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::TreeNodeEx( pPropertyInfo->m_metadata.GetValue( TypeSystem::PropertyMetadata::FriendlyName ).c_str(), ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen );

        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth( -1 );

        void* pPropertyInstance = pPropertyInfo->GetPropertyAddress( pSettings );
        InlineString label( InlineString::CtorSprintf(), "##%s%p", pPropertyInfo->m_metadata.GetValue( TypeSystem::PropertyMetadata::FriendlyName ).c_str(), pPropertyInstance );

        bool wasSettingModified = false;

        TypeSystem::CoreTypeID const coreTypeID = TypeSystem::GetCoreType( pPropertyInfo->m_typeID );
        switch ( coreTypeID )
        {
            case TypeSystem::CoreTypeID::Bool:
            {
                if ( ImGuiX::Checkbox( label.c_str(), (bool*) pPropertyInstance ) )
                {
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::Int8:
            {
                if ( ImGui::InputScalar( label.c_str(), ImGuiDataType_S8, pPropertyInstance ) )
                {
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::Int16:
            {
                if ( ImGui::InputScalar( label.c_str(), ImGuiDataType_S16, pPropertyInstance ) )
                {
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::Int32:
            {
                if ( ImGui::InputScalar( label.c_str(), ImGuiDataType_S32, pPropertyInstance ) )
                {
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::Uint8:
            {
                if ( ImGui::InputScalar( label.c_str(), ImGuiDataType_U8, pPropertyInstance ) )
                {
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::Uint16:
            {
                if ( ImGui::InputScalar( label.c_str(), ImGuiDataType_U16, pPropertyInstance ) )
                {
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::Uint32:
            {

                if ( ImGui::InputScalar( label.c_str(), ImGuiDataType_U32, pPropertyInstance ) )
                {
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::Float:
            {
                if ( ImGui::InputFloat( label.c_str(), (float*) pPropertyInstance ) )
                {
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::String:
            {
                String* pString = (String*) pPropertyInstance;

                auto pScratchBuffer = GetScratchBuffer();
                pScratchBuffer->Fill( *pString );

                bool const wasModified = ImGui::InputText( label.c_str(), pScratchBuffer->Data(), pScratchBuffer->Size() );
                if ( wasModified || ImGui::IsItemDeactivatedAfterEdit() )
                {
                    *( (String*) pPropertyInstance ) = pScratchBuffer->GetString();
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            case TypeSystem::CoreTypeID::StringID:
            {
                StringID* pStringID = (StringID*) pPropertyInstance;

                auto pScratchBuffer = GetScratchBuffer();
                if ( pStringID->IsValid() )
                {
                    pScratchBuffer->Fill( pStringID->c_str() );
                }
                else
                {
                    pScratchBuffer->Clear();
                }

                bool const wasModified = ImGui::InputText( label.c_str(), pScratchBuffer->Data(), pScratchBuffer->Size() );
                if ( wasModified || ImGui::IsItemDeactivatedAfterEdit() )
                {
                    *( (StringID*) pPropertyInstance ) = StringID( pScratchBuffer->GetString() );
                    pSettings->PostLoadOrModify();
                    wasSettingModified = true;
                }
            }
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
        }

        //-------------------------------------------------------------------------

        label.append( "Reset" );

        ImGui::TableNextColumn();
        if ( ImGuiX::FlatIconButton( EE_ICON_UNDO_VARIANT, label.c_str() ) )
        {
            pSettings->GetTypeInfo()->ResetToDefault( pSettings, pPropertyInfo->m_ID );
            pSettings->PostLoadOrModify();
        }
        ImGuiX::ItemTooltip( "Reset Setting to Default" );

        //-------------------------------------------------------------------------

        if ( wasSettingModified )
        {
            m_pSettingsRegistry->SaveSettingsToIniFile();
        }
    }
}