#include "PropertyGrid.h"
#include "PropertyGridEditor.h"
#include "PropertyGridCoreEditors.h"
#include "EngineTools/Core/ToolsContext.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/TypeSystem/PropertyInfo.h"

//-------------------------------------------------------------------------

using namespace EE::TypeSystem;

//-------------------------------------------------------------------------

namespace EE
{
    constexpr static float const    g_extraControlsColumnWidth = 18;
    constexpr static float const    g_extraControlsButtonHeight = 24;
    static const ImVec2             g_extraControlsButtonPadding( 0, 4 );

    //-------------------------------------------------------------------------

    struct [[nodiscard]] ScopedChangeNotifier
    {
        ScopedChangeNotifier( PropertyGrid* pGrid, TypeSystem::PropertyInfo const* pPropertyInfo, PropertyEditInfo::Action action = PropertyEditInfo::Action::Edit )
            : m_pGrid( pGrid )
        {
            m_eventInfo.m_pEditedTypeInstance = m_pGrid->m_pTypeInstance;
            m_eventInfo.m_pPropertyInfo = pPropertyInfo;
            m_eventInfo.m_action = action;
            m_pGrid->m_preEditEvent.Execute( m_eventInfo );
        }

        ~ScopedChangeNotifier()
        {
            m_eventInfo.m_pEditedTypeInstance->PostPropertyEdit( m_eventInfo.m_pPropertyInfo );
            m_pGrid->m_postEditEvent.Execute( m_eventInfo );
            m_pGrid->m_isDirty = true;
        }

        PropertyGrid*                       m_pGrid = nullptr;
        PropertyEditInfo                    m_eventInfo;
    };

    //-------------------------------------------------------------------------

    static InlineString GetFriendlyTypename( TypeRegistry const& typeRegistry, TypeID typeID )
    {
        EE_ASSERT( typeID );
        InlineString friendlyTypeName;

        // Get core type friendly name
        CoreTypeID const coreTypeID = CoreTypeRegistry::GetType( typeID );
        if ( coreTypeID != CoreTypeID::Invalid )
        {
            friendlyTypeName = CoreTypeRegistry::GetFriendlyName( coreTypeID );
        }
        else // Read type info from type registry
        {
            auto pTypeInfo = typeRegistry.GetTypeInfo( typeID );
            EE_ASSERT( pTypeInfo != nullptr );

            if ( pTypeInfo->HasCategoryName() )
            {
                friendlyTypeName = InlineString( pTypeInfo->GetCategoryName() ) + "/" + pTypeInfo->m_friendlyName.c_str();
            }
            else
            {
                friendlyTypeName = InlineString( pTypeInfo->m_friendlyName.c_str() );
            }
        }

        return friendlyTypeName;
    }

    //-------------------------------------------------------------------------

    PropertyGrid::PropertyGrid( ToolsContext const* pToolsContext )
        : m_pToolsContext( pToolsContext )
        , m_resourcePicker( *pToolsContext )
    {
        EE_ASSERT( m_pToolsContext != nullptr && m_pToolsContext->IsValid() );
    }

    PropertyGrid::~PropertyGrid()
    {
        DestroyPropertyEditors();
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::SetTypeToEdit( IRegisteredType* pTypeInstance )
    {
        if ( pTypeInstance == nullptr )
        {
            SetTypeToEdit( nullptr );
        }
        else if( pTypeInstance != m_pTypeInstance )
        {
            m_pTypeInfo = pTypeInstance->GetTypeInfo();
            m_pTypeInstance = pTypeInstance;
            DestroyPropertyEditors();
            m_isDirty = false;
        }
    }

    void PropertyGrid::SetTypeToEdit( nullptr_t )
    {
        m_pTypeInfo = nullptr;
        m_pTypeInstance = nullptr;
        m_isDirty = false;

        DestroyPropertyEditors();
    }

    //-------------------------------------------------------------------------

    PropertyEditor* PropertyGrid::GetPropertyEditor( PropertyInfo const& propertyInfo, uint8_t* pActualPropertyInstance )
    {
        PropertyEditor* pPropertyEditor = nullptr;

        auto foundIter = m_propertyEditors.find( pActualPropertyInstance );
        if ( foundIter != m_propertyEditors.end() )
        {
            pPropertyEditor = foundIter->second;
        }
        else // Create new editor instance
        {
            pPropertyEditor = CreatePropertyEditor( m_pToolsContext, m_resourcePicker, propertyInfo, pActualPropertyInstance );
            m_propertyEditors[pActualPropertyInstance] = pPropertyEditor;
        }

        return pPropertyEditor;
    }

    void PropertyGrid::DestroyPropertyEditors()
    {
        for ( auto& pair : m_propertyEditors )
        {
            EE::Delete( pair.second );
        }

        m_propertyEditors.clear();
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::ExpandAllPropertyViews()
    {
        // TODO
    }

    void PropertyGrid::CollapseAllPropertyViews()
    {
        // TODO
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::DrawGrid()
    {
        if ( m_pTypeInstance == nullptr )
        {
            ImGui::Text( "Nothing To Edit." );
            return;
        }

        // Control Bar
        //-------------------------------------------------------------------------

        if ( m_isControlBarVisible )
        {
            if ( m_showAllRegisteredProperties )
            {
                if ( ImGuiX::FlatButton( EE_ICON_EYE_OUTLINE"##ToggleShowRegisteredProperties" ) )
                {
                    m_showAllRegisteredProperties = false;
                }
                ImGuiX::ItemTooltip( "Show only exposed properties" );
            }
            else
            {
                if ( ImGuiX::FlatButton( EE_ICON_EYE_OFF_OUTLINE"##ToggleShowRegisteredProperties" ) )
                {
                    m_showAllRegisteredProperties = true;
                }
                ImGuiX::ItemTooltip( "Show all properties" );
            }
        }

        // Properties
        //-------------------------------------------------------------------------

        ImGuiTableFlags const flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit;
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 2, 4 ) );
        if ( ImGui::BeginTable( "PropertyGrid", 3, flags ) )
        {
            ImGui::TableSetupColumn( "##Property", ImGuiTableColumnFlags_WidthFixed, 100 );
            ImGui::TableSetupColumn( "##EditorWidget", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "##ExtraControls", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, g_extraControlsColumnWidth );

            //-------------------------------------------------------------------------

            for ( auto const& propertyInfo : m_pTypeInfo->m_properties )
            {
                if ( !m_showAllRegisteredProperties && !propertyInfo.IsExposedProperty() )
                {
                    continue;
                }

                ImGui::TableNextRow();
                DrawPropertyRow( m_pTypeInfo, m_pTypeInstance, propertyInfo, reinterpret_cast<uint8_t*>( m_pTypeInstance ) + propertyInfo.m_offset );
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }

    void PropertyGrid::DrawPropertyRow( TypeSystem::TypeInfo const* pTypeInfo, IRegisteredType* pTypeInstance, PropertyInfo const& propertyInfo, uint8_t* pPropertyInstance )
    {
        if ( propertyInfo.IsArrayProperty() )
        {
            DrawArrayPropertyRow( pTypeInfo, pTypeInstance, propertyInfo, pPropertyInstance );
        }
        else
        {
            DrawValuePropertyRow( pTypeInfo, pTypeInstance, propertyInfo, pPropertyInstance );
        }
    }

    void PropertyGrid::DrawValuePropertyRow( TypeSystem::TypeInfo const* pTypeInfo, IRegisteredType* pTypeInstance, PropertyInfo const& propertyInfo, uint8_t* pPropertyInstance, int32_t arrayIdx )
    {
        //-------------------------------------------------------------------------
        // Name
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();

        InlineString propertyName;
        if ( arrayIdx != InvalidIndex )
        {
            propertyName.sprintf( "%d", arrayIdx );
        }
        else
        {
            propertyName = propertyInfo.m_friendlyName.c_str();
        }

        bool showContents = false;
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

            if ( propertyInfo.IsStructureProperty() )
            {
                if ( ImGui::TreeNodeEx( propertyName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth ) )
                {
                    showContents = true;
                }
            }
            else
            {
                ImGui::Text( propertyName.c_str() );
            }

            // Description
            if ( !propertyInfo.m_description.empty() && ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( propertyInfo.m_description.c_str() );
            }
        }

        //-------------------------------------------------------------------------

        auto pActualPropertyInstance = propertyInfo.IsArrayProperty() ? pTypeInfo->GetArrayElementDataPtr( pTypeInstance, propertyInfo.m_ID, arrayIdx ) : pPropertyInstance;

        //-------------------------------------------------------------------------
        // Editor
        //-------------------------------------------------------------------------

        PropertyEditor* pPropertyEditor = GetPropertyEditor( propertyInfo, pActualPropertyInstance );

        ImGui::TableNextColumn();
        
        if ( propertyInfo.IsStructureProperty() )
        {
            InlineString const friendlyTypeName = GetFriendlyTypename( *m_pToolsContext->m_pTypeRegistry, propertyInfo.m_typeID );
            ImGui::TextColored( Colors::Gray.ToFloat4(), friendlyTypeName.c_str() );
        }
        else // Create property editor
        {
            if ( pPropertyEditor != nullptr )
            {
                ImGui::BeginDisabled( !propertyInfo.IsExposedProperty() );
                if ( pPropertyEditor->UpdateAndDraw() )
                {
                    ScopedChangeNotifier cn( this, &propertyInfo );
                    pPropertyEditor->UpdatePropertyValue();
                }
                ImGui::EndDisabled();

                if ( !propertyInfo.m_description.empty() )
                {
                    ImGuiX::ItemTooltip( propertyInfo.m_description.c_str() );
                }
            }
            else
            {
                ImGui::Text( "No Editor Found!" );
            }
        }

        //-------------------------------------------------------------------------
        // Controls
        //-------------------------------------------------------------------------

        enum class Command { None, ResetToDefault, RemoveElement };
        Command command = Command::None;

        ImGui::TableNextColumn();

        ImGui::PushID( pActualPropertyInstance );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_extraControlsButtonPadding );
        ImGui::BeginDisabled( !propertyInfo.IsExposedProperty() );
        if ( propertyInfo.IsDynamicArrayProperty() )
        {
            EE_ASSERT( arrayIdx != InvalidIndex );
            if ( ImGuiX::FlatButtonColored( Colors::PaleVioletRed.ToFloat4(), EE_ICON_MINUS, ImVec2( g_extraControlsColumnWidth, g_extraControlsButtonHeight ) ) )
            {
                command = Command::RemoveElement;
            }
            ImGuiX::ItemTooltip( "Remove array element" );
        }
        else if ( !pTypeInfo->IsPropertyValueSetToDefault( pTypeInstance, propertyInfo.m_ID, arrayIdx ) )
        {
            if ( ImGuiX::FlatButtonColored( Colors::LightGray.ToFloat4(), EE_ICON_UNDO_VARIANT, ImVec2( g_extraControlsColumnWidth, g_extraControlsButtonHeight ) ) )
            {
                command = Command::ResetToDefault;
            }
            ImGuiX::ItemTooltip( "Reset value to default" );
        }
        ImGui::EndDisabled();
        ImGui::PopStyleVar();
        ImGui::PopID();

        //-------------------------------------------------------------------------
        // Child Properties
        //-------------------------------------------------------------------------
        // Only relevant for structure properties

        if ( showContents )
        {
            EE_ASSERT( propertyInfo.IsStructureProperty() );

            TypeInfo const* pChildTypeInfo = m_pToolsContext->m_pTypeRegistry->GetTypeInfo( propertyInfo.m_typeID );
            EE_ASSERT( pChildTypeInfo != nullptr );
            uint8_t* pChildTypeInstance = pActualPropertyInstance;

            for ( auto const& childPropertyInfo : pChildTypeInfo->m_properties )
            {
                if ( !m_showAllRegisteredProperties && !propertyInfo.IsExposedProperty() )
                {
                    continue;
                }

                ImGui::TableNextRow();
                DrawPropertyRow( pChildTypeInfo, reinterpret_cast<IRegisteredType*>( pChildTypeInstance ), childPropertyInfo, pChildTypeInstance + childPropertyInfo.m_offset );
            }

            ImGui::TreePop();
        }

        //-------------------------------------------------------------------------
        // Handle control requests
        //-------------------------------------------------------------------------
        // This needs to be done after we have finished drawing the UI

        switch ( command ) 
        {
            case Command::RemoveElement:
            {
                ScopedChangeNotifier cn( this, &propertyInfo, PropertyEditInfo::Action::RemoveArrayElement );
                pTypeInfo->RemoveArrayElement( pTypeInstance, propertyInfo.m_ID, arrayIdx );
            }
            break;

            case Command::ResetToDefault:
            {
                ScopedChangeNotifier cn( this, &propertyInfo );
                pTypeInfo->ResetToDefault( pTypeInstance, propertyInfo.m_ID );

                if ( pPropertyEditor != nullptr )
                {
                    pPropertyEditor->ResetWorkingCopy();
                }
            }
            break;

            default:
            break;
        }
    }

    void PropertyGrid::DrawArrayPropertyRow( TypeSystem::TypeInfo const* pTypeInfo, IRegisteredType* pTypeInstance, PropertyInfo const& propertyInfo, uint8_t* pPropertyInstance )
    {
        EE_ASSERT( propertyInfo.IsArrayProperty() );

        ImGui::PushID( pPropertyInstance );

        // Name
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();

        bool showContents = false;
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
            if ( ImGui::TreeNodeEx( propertyInfo.m_friendlyName.c_str(), ImGuiTreeNodeFlags_None ) )
            {
                showContents = true;
            }
        }

        // Description
        if ( !propertyInfo.m_description.empty() && ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( propertyInfo.m_description.c_str() );
        }

        // Editor
        //-------------------------------------------------------------------------

        InlineString const friendlyTypeName = GetFriendlyTypename( *m_pToolsContext->m_pTypeRegistry, propertyInfo.m_typeID );
        size_t const arraySize = pTypeInfo->GetArraySize( pTypeInstance, propertyInfo.m_ID );

        ImGui::TableNextColumn();
        if ( propertyInfo.IsDynamicArrayProperty() )
        {
            float const cellContentWidth = ImGui::GetContentRegionAvail().x;
            float const itemSpacing = ImGui::GetStyle().ItemSpacing.x / 2;
            float const buttonAreaWidth = 21;
            float const textAreaWidth = cellContentWidth - buttonAreaWidth - itemSpacing;

            ImGui::AlignTextToFramePadding();
            ImGui::TextColored( Colors::Gray.ToFloat4(), "%d Elements - %s", arraySize, friendlyTypeName.c_str() );
            float const actualTextWidth = ImGui::GetItemRectSize().x;

            ImGui::SameLine( 0, textAreaWidth - actualTextWidth + itemSpacing );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_extraControlsButtonPadding );
            ImGui::BeginDisabled( !propertyInfo.IsExposedProperty() );
            if ( ImGuiX::FlatButtonColored( Colors::LightGreen.ToFloat4(), EE_ICON_PLUS, ImVec2( g_extraControlsColumnWidth, g_extraControlsButtonHeight ) ) )
            {
                ScopedChangeNotifier cn( this, &propertyInfo, PropertyEditInfo::Action::AddArrayElement );
                pTypeInfo->AddArrayElement( pTypeInstance, propertyInfo.m_ID );
                ImGui::GetStateStorage()->SetInt( ImGui::GetID( propertyInfo.m_ID.c_str() ), 1 );
            }
            ImGui::EndDisabled();
            ImGuiX::ItemTooltip( "Add array element" );
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored( Colors::Gray.ToFloat4(), "%d Elements - %s", arraySize, friendlyTypeName.c_str() );
        }

        // Extra Controls
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_extraControlsButtonPadding );
        if ( !pTypeInfo->IsPropertyValueSetToDefault( pTypeInstance, propertyInfo.m_ID ) )
        {
            ImGui::BeginDisabled( !propertyInfo.IsExposedProperty() );
            if ( ImGuiX::FlatButtonColored( Colors::LightGray.ToFloat4(), EE_ICON_UNDO_VARIANT, ImVec2( g_extraControlsColumnWidth, g_extraControlsButtonHeight ) ) )
            {
                ScopedChangeNotifier cn( this, &propertyInfo );
                pTypeInfo->ResetToDefault( pTypeInstance, propertyInfo.m_ID );
            }
            ImGui::EndDisabled();
            ImGuiX::ItemTooltip( "Reset value to default" );
        }
        ImGui::PopStyleVar();

        // Array Elements
        //-------------------------------------------------------------------------

        if ( showContents )
        {
            // We need to ask for the array size each iteration since we may destroy a row as part of drawing it
            for ( auto i = 0u; i < pTypeInfo->GetArraySize( pTypeInstance, propertyInfo.m_ID ); i++ )
            {
                DrawValuePropertyRow( pTypeInfo, pTypeInstance, propertyInfo, pPropertyInstance, i );
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}