#include "TypeInfoPicker.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE
{
    TypeInfoPicker::TypeInfoPicker( ToolsContext const& toolsContext, TypeSystem::TypeID const& baseClassTypeID, TypeSystem::TypeInfo const* pSelectedTypeInfo )
        : m_toolsContext( toolsContext )
        , m_baseClassTypeID( baseClassTypeID )
        , m_pSelectedTypeInfo( pSelectedTypeInfo )
    {
        if ( m_baseClassTypeID.IsValid() && pSelectedTypeInfo != nullptr )
        {
            EE_ASSERT( pSelectedTypeInfo->IsDerivedFrom( m_baseClassTypeID ) );
        }

        m_filterWidget.SetFilterHelpText( "Filter Types" );

        GenerateOptionsList();
        GenerateFilteredOptionList();
    }

    bool TypeInfoPicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        if ( m_isPickerDisabled )
        {
            InlineString typeInfoLabel;
            if ( m_pSelectedTypeInfo != nullptr )
            {
                typeInfoLabel.sprintf( ConstructFriendlyTypeInfoName( m_pSelectedTypeInfo ).c_str() );
            }
            else
            {
                typeInfoLabel = "No Type selected";
            }

            ImGui::SetNextItemWidth( -1 );
            ImGui::InputText( "##typeInfoLabel", const_cast<char*>( typeInfoLabel.c_str() ), typeInfoLabel.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
        }
        else
        {
            ImGui::PushID( this );
            auto const& style = ImGui::GetStyle();
            float const itemSpacingX = style.ItemSpacing.x;
            float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
            float const buttonWidth = ImGui::GetFrameHeight() + style.ItemSpacing.x;

            // Type Label
            //-------------------------------------------------------------------------

            InlineString typeInfoLabel;
            if ( m_pSelectedTypeInfo != nullptr )
            {
                typeInfoLabel.sprintf( ConstructFriendlyTypeInfoName( m_pSelectedTypeInfo ).c_str() );
            }
            else
            {
                typeInfoLabel = "No Type selected";
            }

            ImGui::SetNextItemWidth( contentRegionAvailableX - ( buttonWidth * 2 ) - style.ItemSpacing.x );
            ImGui::InputText( "##typeInfoLabel", const_cast<char*>( typeInfoLabel.c_str() ), typeInfoLabel.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );

            // Combo
            //-------------------------------------------------------------------------

            ImVec2 const comboDropDownSize( Math::Max( contentRegionAvailableX, 500.0f ), ImGui::GetFrameHeight() * 20 );
            ImGui::SetNextWindowSizeConstraints( ImVec2( comboDropDownSize.x, 0 ), comboDropDownSize );

            ImGui::SameLine();

            bool const wasComboOpen = m_isComboOpen;
            m_isComboOpen = ImGui::BeginCombo( "##TypeInfoCombo", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview );

            // Draw combo if open
            bool shouldUpdateNavID = false;
            if ( m_isComboOpen )
            {
                float const cursorPosYPreFilter = ImGui::GetCursorPosY();
                if ( m_filterWidget.UpdateAndDraw( -1, ImGuiX::FilterWidget::Flags::TakeInitialFocus ) )
                {
                    GenerateFilteredOptionList();
                }

                float const cursorPosYPostHeaderItems = ImGui::GetCursorPosY();
                ImVec2 const previousCursorPos = ImGui::GetCursorPos();
                ImVec2 const childSize( ImGui::GetContentRegionAvail().x, comboDropDownSize.y - cursorPosYPostHeaderItems - style.ItemSpacing.y - style.WindowPadding.y );
                ImGui::Dummy( childSize );
                ImGui::SetCursorPos( previousCursorPos );

                //-------------------------------------------------------------------------

                InlineString selectableLabel;
                {
                    ImGuiX::ScopedFont const sfo( ImGuiX::Font::Medium );

                    ImGuiListClipper clipper;
                    clipper.Begin( (int32_t) m_filteredOptions.size() );
                    while ( clipper.Step() )
                    {
                        for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                        {
                            EE_ASSERT( m_filteredOptions[i].m_pTypeInfo != nullptr );

                            selectableLabel.sprintf( "%s##%d", m_filteredOptions[i].m_friendlyName.c_str(), m_filteredOptions[i].m_pTypeInfo->m_ID.ToUint() );
                            if ( ImGui::Selectable( selectableLabel.c_str() ) )
                            {
                                m_pSelectedTypeInfo = m_filteredOptions[i].m_pTypeInfo;
                                valueUpdated = true;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGuiX::ItemTooltip( m_filteredOptions[i].m_friendlyName.c_str() );
                        }
                    }
                    clipper.End();
                }

                ImGui::EndCombo();
            }

            // Clear Button
            //-------------------------------------------------------------------------

            ImGui::SameLine();

            ImGui::BeginDisabled( m_pSelectedTypeInfo == nullptr );
            if ( ImGui::Button( EE_ICON_TRASH_CAN"##Clear", ImVec2( buttonWidth, 0 ) ) )
            {
                m_pSelectedTypeInfo = nullptr;
                valueUpdated = true;
            }
            ImGuiX::ItemTooltip( "Clear Selected Type" );
            ImGui::EndDisabled();
            ImGui::PopID();
        }

        //-------------------------------------------------------------------------

        return valueUpdated;
    }

    void TypeInfoPicker::SetRequiredBaseClass( TypeSystem::TypeID baseClassTypeID )
    {
        m_baseClassTypeID = baseClassTypeID;
        GenerateOptionsList();
        GenerateFilteredOptionList();
    }

    void TypeInfoPicker::SetSelectedType( TypeSystem::TypeInfo const* pSelectedTypeInfo )
    {
        if ( m_baseClassTypeID.IsValid() && pSelectedTypeInfo != nullptr )
        {
            EE_ASSERT( pSelectedTypeInfo->IsDerivedFrom( m_baseClassTypeID ) );
            m_pSelectedTypeInfo = pSelectedTypeInfo;
        }
    }

    void TypeInfoPicker::SetSelectedType( TypeSystem::TypeID typeID )
    {
        if ( typeID.IsValid() )
        {
            m_pSelectedTypeInfo = m_toolsContext.m_pTypeRegistry->GetTypeInfo( typeID );
        }
        else
        {
            m_pSelectedTypeInfo = nullptr;
        }
    }

    void TypeInfoPicker::GenerateOptionsList()
    {
        m_generatedOptions.clear();

        TVector<TypeSystem::TypeInfo const*> validTypes;
        if ( m_baseClassTypeID.IsValid() )
        {
            validTypes = m_toolsContext.m_pTypeRegistry->GetAllDerivedTypes( m_baseClassTypeID, true, false, true );
        }
        else
        {
            validTypes = m_toolsContext.m_pTypeRegistry->GetAllTypes( false, true );
        }

        for ( auto pTypeInfo : validTypes )
        {
            auto& newOption = m_generatedOptions.emplace_back();
            newOption.m_pTypeInfo = pTypeInfo;
            newOption.m_friendlyName = ConstructFriendlyTypeInfoName( pTypeInfo );

            // Generate a string that will allow us to filter the list
            newOption.m_filterableData.sprintf( "%s %s", pTypeInfo->m_category.c_str(), newOption.m_friendlyName.c_str() );
            StringUtils::RemoveAllOccurrencesInPlace( newOption.m_filterableData, ":" );
            StringUtils::RemoveAllOccurrencesInPlace( newOption.m_filterableData, "/" );
            newOption.m_filterableData.make_lower();
        }
    }

    void TypeInfoPicker::GenerateFilteredOptionList()
    {
        if ( m_filterWidget.HasFilterSet() )
        {
            m_filteredOptions.clear();

            for ( Option const& option : m_generatedOptions )
            {
                if ( m_filterWidget.MatchesFilter( option.m_filterableData ) )
                {
                    m_filteredOptions.emplace_back( option );
                }
            }
        }
        else
        {
            m_filteredOptions = m_generatedOptions;
        }
    }

    String TypeInfoPicker::ConstructFriendlyTypeInfoName( TypeSystem::TypeInfo const* pTypeInfo )
    {
        String friendlyName;

        if ( pTypeInfo->m_namespace.empty() )
        {
            friendlyName = pTypeInfo->m_friendlyName;
        }
        else
        {
            friendlyName.sprintf( "%s::%s", pTypeInfo->m_namespace.c_str(), pTypeInfo->m_friendlyName.c_str() );
        }

        return friendlyName;
    }
}