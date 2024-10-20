#include "PropertyGrid.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/TypeSystem/PropertyInfo.h"
#include "PropertyGridEditor.h"
#include "PropertyGridTypeEditingRules.h"
#include "Base/TypeSystem/TypeInstance.h"

//-------------------------------------------------------------------------

namespace EE::PG
{
    struct GridContext : public PropertyEditorContext
    {
        PropertyGrid*                                       m_pPropertyGrid = nullptr;

        mutable PropertyGrid::VisualState const*            m_pRecordedVisualState = nullptr; // Only set during grid rebuild, to restore expansion and other visual state
        mutable GridRow*                                    m_pRowThatRequiresUpdateAndRebuild = nullptr; // Set whenever we get an type update request from a row that requires us to update the row and rebuild the grid
    };

    // This struct wraps a property modification operation
    struct [[nodiscard]] ScopedChangeNotifier
    {
        ScopedChangeNotifier( PropertyGrid* pGrid, GridRow const* pRow, IReflectedType* pTypeInstance, TypeSystem::PropertyInfo const* pPropertyInfo, PropertyEditInfo::Action action = PropertyEditInfo::Action::Edit )
            : m_pGrid( pGrid )
        {
            EE_ASSERT( pGrid != nullptr );
            EE_ASSERT( pTypeInstance != nullptr );
            m_eventInfo.m_pOwnerTypeInstance = pGrid->m_pTypeInstance;
            m_eventInfo.m_pTypeInstanceEdited = pTypeInstance;
            m_eventInfo.m_pPropertyInfo = pPropertyInfo;
            m_eventInfo.m_action = action;
            m_pGrid->m_preEditEvent.Execute( m_eventInfo );

            // Get the parent property chain
            GridRow const* pParentRow = pRow->GetParent();
            if ( pParentRow != nullptr )
            {
                pParentRow->GeneratePropertyChangedNotificationChain( m_propertyChain );
            }

            // Notify the type that we've modified a property
            // This is explicitly done to handle array modifications that will not end up in the property chain
            m_eventInfo.m_pTypeInstanceEdited->PrePropertyEdit( m_eventInfo.m_pPropertyInfo );

            // Notify all parent types in the property chain that there's been a modification
            for ( auto iter = m_propertyChain.rbegin(); iter != m_propertyChain.rend(); ++iter )
            {
                iter->m_pTypeInstance->PrePropertyEdit( iter->m_pPropertyInfo );
            }
        }

        ~ScopedChangeNotifier()
        {
            // Notify the type that we've modified a property
            // This is explicitly done to handle array modifications that will not end up in the property chain
            m_eventInfo.m_pTypeInstanceEdited->PostPropertyEdit( m_eventInfo.m_pPropertyInfo );

            // Notify all parent types in the property chain that there's been a modification
            for ( auto iter = m_propertyChain.rbegin(); iter != m_propertyChain.rend(); ++iter )
            {
                iter->m_pTypeInstance->PostPropertyEdit( iter->m_pPropertyInfo );
            }

            m_pGrid->m_postEditEvent.Execute( m_eventInfo );
            m_pGrid->m_isDirty = true;
            m_pGrid->ApplyFilter();
        }

        PropertyGrid*                       m_pGrid = nullptr;
        TVector<PropertyChainElement>       m_propertyChain;
        PropertyEditInfo                    m_eventInfo;
    };
}

//-------------------------------------------------------------------------

namespace EE
{
    using namespace TypeSystem;

    static InlineString GetFriendlyTypename( TypeRegistry const& typeRegistry, TypeID typeID )
    {
        EE_ASSERT( typeID.IsValid() );
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
    {
        EE_ASSERT( pToolsContext != nullptr && pToolsContext->IsValid() );

        m_pGridContext = EE::New<PG::GridContext>();
        m_pGridContext->m_pPropertyGrid = this;
        m_pGridContext->m_pToolsContext = pToolsContext;
    }

    PropertyGrid::~PropertyGrid()
    {
        RebuildGrid( nullptr, nullptr );
        EE::Delete( m_pGridContext );
    }

    void PropertyGrid::SetUserContext( void* pContext )
    {
        m_pGridContext->m_pUserContext = pContext;
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::SetTypeToEdit( IReflectedType* pTypeInstance, VisualState const* pVisualStateToRestore )
    {
        RebuildGrid( pTypeInstance, pVisualStateToRestore );
        m_isDirty = false;
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::RebuildGrid( IReflectedType* pTypeInstance, VisualState const* pVisualStateToRestore )
    {
        // Set type to edit
        //-------------------------------------------------------------------------

        m_pTypeInstance = pTypeInstance;
        m_pTypeInfo = ( m_pTypeInstance != nullptr ) ? pTypeInstance->GetTypeInfo() : nullptr;

        //-------------------------------------------------------------------------

        for ( auto& pCategory : m_categories )
        {
            pCategory->DestroyChildren();
            EE::Delete( pCategory );
        }

        m_categories.clear();

        //-------------------------------------------------------------------------

        if ( m_pTypeInfo == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_pGridContext->m_pRecordedVisualState = pVisualStateToRestore;

        for ( auto const& propertyInfo : m_pTypeInfo->m_properties )
        {
            if ( propertyInfo.m_metadata.HasFlag( PropertyMetadata::Flag::Hidden ) )
            {
                continue;
            }

            auto const* pKV = propertyInfo.m_metadata.TryGetEntryForFlag( PropertyMetadata::Flag::Category );
            String const category = ( pKV == nullptr || pKV->m_value.empty() ) ? "General" : pKV->m_value;
            PG::CategoryRow* pCategory = PG::CategoryRow::FindOrCreateCategory( nullptr, *m_pGridContext, m_categories, category );
            pCategory->AddProperty( m_pTypeInstance, propertyInfo );
        }

        m_pGridContext->m_pRecordedVisualState = nullptr;

        //-------------------------------------------------------------------------

        if ( pVisualStateToRestore != nullptr )
        {
            m_scrollPosY = pVisualStateToRestore->m_scrollPosY;
            m_restoreScrollPosY = true;
        }

        //-------------------------------------------------------------------------

        ApplyFilter();
    }

    void PropertyGrid::ApplyFilter()
    {
        auto const& filters = m_filterWidget.GetFilterTokens();
        auto EvaluateRowFilter = [this, &filters]( PG::GridRow* pRow )
        {
            // Optionally show read-only properties
            if ( !pRow->IsDeclaredHidden() )
            {
                // Reset Visibility
                pRow->SetHidden( false );

                // Apply read-only visibility filter
                if ( pRow->IsReadOnly() )
                {
                    pRow->SetHidden( !m_showReadOnlyProperties );
                }
            }

            // Apply name filter
            if ( !pRow->IsHidden() && !filters.empty() )
            {
                String rowNameLowercase = pRow->GetName();
                rowNameLowercase.make_lower();

                bool shouldRowBeHidden = true;
                for ( auto const& filter : filters )
                {
                    if ( rowNameLowercase.find( filter ) != String::npos )
                    {
                        shouldRowBeHidden = false;
                        break;
                    }
                }

                pRow->SetHidden( shouldRowBeHidden );
            }
        };

        for ( auto& pCategory : m_categories )
        {
            pCategory->RecursiveOperation( EvaluateRowFilter );
        }
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::ExpandAll()
    {
        for ( auto& pCategory : m_categories )
        {
            pCategory->SetExpansion( true );
        }
    }

    void PropertyGrid::CollapseAll()
    {
        for ( auto& pCategory : m_categories )
        {
            pCategory->SetExpansion( false );
        }
    }

    void PropertyGrid::GetCurrentVisualState( VisualState& outVisualState )
    {
        outVisualState.Clear();

        if ( m_pTypeInstance == nullptr )
        {
            return;
        }

        // Record some extra info about what we are currently editing
        outVisualState.m_editedTypeID = m_pTypeInstance->GetTypeID();

        // Get expansion state
        for ( auto& pCategory : m_categories )
        {
            if ( pCategory->IsExpanded() )
            {
                outVisualState.m_expandedCategories.emplace_back( pCategory->GetName() );
            }

            pCategory->FillExpansionInfo( outVisualState );
        }

        // Get scroll state
        outVisualState.m_scrollPosY = m_scrollPosY;
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::UpdateAndDraw()
    {
        // Do we have a rebuild request?
        //-------------------------------------------------------------------------

        if ( m_pGridContext->m_pRowThatRequiresUpdateAndRebuild != nullptr )
        {
            m_pGridContext->m_pRowThatRequiresUpdateAndRebuild->UpdateRow();
            m_pGridContext->m_pRowThatRequiresUpdateAndRebuild = nullptr;

            VisualState visualState;
            GetCurrentVisualState( visualState );
            RebuildGrid( m_pTypeInstance, &visualState );
        }

        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( m_isReadOnly );

        // Control Bar
        //-------------------------------------------------------------------------

        if ( m_isControlBarVisible )
        {
            constexpr float const buttonWidth = 30;
            float const filterWidth = ImGui::GetContentRegionAvail().x - ( 3 * ( buttonWidth + ImGui::GetStyle().ItemSpacing.x ) );
            if ( m_filterWidget.UpdateAndDraw( filterWidth ) )
            {
                ApplyFilter();
            }

            ImGui::SameLine();

            if ( ImGui::Button( EE_ICON_COLLAPSE_ALL"##CollapseAll", ImVec2( buttonWidth, 0 ) ) )
            {
                CollapseAll();
            }
            ImGuiX::ItemTooltip( "Collapse All" );

            ImGui::SameLine();

            if ( ImGui::Button( EE_ICON_EXPAND_ALL"##ExpandAll" ) )
            {
                ExpandAll();
            }
            ImGuiX::ItemTooltip( "Expand All" );

            ImGui::SameLine();

            if ( ImGuiX::ToggleButton( EE_ICON_EYE_OUTLINE"##ToggleShowEditableProperties", EE_ICON_EYE_OFF_OUTLINE"##ToggleShowEditableProperties", m_showReadOnlyProperties ) )
            {
                ApplyFilter();
            }
            ImGuiX::ItemTooltip( "Read-only property visibility" );
        }

        // Grid Rows
        //-------------------------------------------------------------------------

        float const tableHeight = Math::Max( 0.0f, m_fillRemainingSpace ? ImGui::GetContentRegionAvail().y - 1 : Math::Max( s_absoluteMinimumHeight, m_minimumHeight ) );
        ImVec2 const tableSize = ImVec2( ImGui::GetContentRegionAvail().x - 1, tableHeight );
        ImGuiTableFlags const flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 8 ) );
        ImGui::PushID( this );
        if ( ImGui::BeginTable( "GridTable", 2, flags, tableSize ) )
        {
            ImGui::TableSetupColumn( "##Header", ImGuiTableColumnFlags_WidthFixed, 200 );
            ImGui::TableSetupColumn( "##Editor", ImGuiTableColumnFlags_WidthStretch );

            //-------------------------------------------------------------------------

            if ( m_pTypeInstance != nullptr )
            {
                for ( auto& pCategory : m_categories )
                {
                    pCategory->UpdateRow();
                    pCategory->DrawRow( 0 );
                }
            }

            //-------------------------------------------------------------------------

            if ( m_restoreScrollPosY )
            {
                ImGui::SetScrollY( m_scrollPosY );
                m_restoreScrollPosY = false;
            }
            else
            {
                m_scrollPosY = ImGui::GetScrollY();
            }

            ImGui::EndTable();
        }
        ImGui::PopID();
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------

        ImGui::EndDisabled();
    }
}

//-------------------------------------------------------------------------

namespace EE::PG
{
    constexpr float const g_headerOffset = 20;
    static ImVec2 const g_controlButtonSize( 18, 24 );
    static ImVec2 const g_controlButtonPadding( 0, 4 );

    //-------------------------------------------------------------------------

    static GridRow* CreateRow( GridRow* pParentRow, GridContext const& context, IReflectedType* pTypeInstance, PropertyInfo const& propertyInfo, int32_t arrayElementIdx = InvalidIndex )
    {
        GridRow* pRow = nullptr;

        if ( propertyInfo.IsArrayProperty() && arrayElementIdx == InvalidIndex )
        {
            pRow = EE::New<ArrayRow>( pParentRow, context, propertyInfo, pTypeInstance );
        }
        else
        {
            pRow = EE::New<PropertyRow>( pParentRow, context, propertyInfo, pTypeInstance, arrayElementIdx );
        }

        return pRow;
    }

    //-------------------------------------------------------------------------

    void GridRow::UpdateRow()
    {
        // Children are updated before parents so parents can inspect child state
        for ( auto& child : m_children )
        {
            child->UpdateRow();
        }

        // Perform Update
        Update();
        UpdateName();
    }

    void GridRow::DrawRow( float currentHeaderOffset )
    {
        //-------------------------------------------------------------------------
        // Draw
        //-------------------------------------------------------------------------

        bool const shouldDrawRow = ShouldDrawRow();
        if ( shouldDrawRow )
        {
            ImGui::PushID( this );
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                DrawHeaderSection( currentHeaderOffset );

                ImGui::TableNextColumn();
                {
                    ImGuiTableFlags const flags = ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingFixedFit;
                    ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 2, 0 ) );
                    if ( ImGui::BeginTable( "GridTable", HasExtraControls() ? 3 : 2, flags ) )
                    {
                        ImGui::TableSetupColumn( "##Editor", ImGuiTableColumnFlags_WidthStretch );

                        if ( HasExtraControls() )
                        {
                            ImGui::TableSetupColumn( "##Extra", ImGuiTableColumnFlags_WidthFixed, GetExtraControlsSectionWidth() );
                        }

                        ImGui::TableSetupColumn( "##Reset", ImGuiTableColumnFlags_WidthFixed, g_controlButtonSize.x );

                        //-------------------------------------------------------------------------

                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        DrawEditorSection();

                        if ( HasExtraControls() )
                        {
                            ImGui::TableNextColumn();
                            DrawExtraControlsSection();
                        }

                        ImGui::TableNextColumn();
                        if ( HasResetSection() )
                        {
                            DrawResetSection();
                        }

                        ImGui::EndTable();
                    }
                    ImGui::PopStyleVar();
                }
            }
            ImGui::PopID();
        }

        //-------------------------------------------------------------------------
        // Draw Children
        //-------------------------------------------------------------------------

        if ( m_isExpanded )
        {
            ImGui::BeginDisabled( IsReadOnly() );

            float const childHeaderOffset = currentHeaderOffset + g_headerOffset;
            for ( auto& child : m_children )
            {
                child->DrawRow( childHeaderOffset );
            }

            ImGui::EndDisabled();
        }
    }

    void GridRow::SetExpansion( bool isExpanded )
    {
        m_isExpanded = isExpanded;

        for ( auto pChild : m_children )
        {
            pChild->SetExpansion( isExpanded );
        }
    }

    void GridRow::DestroyChildren()
    {
        for ( auto pChild : m_children )
        {
            pChild->DestroyChildren();
            EE::Delete( pChild );
        }

        m_children.clear();
    }

    void GridRow::GeneratePropertyChangedNotificationChain( TVector<PropertyChainElement>& outChain ) const
    {
        if ( m_pParent != nullptr )
        {
            m_pParent->GeneratePropertyChangedNotificationChain( outChain );
        }
    }

    void GridRow::FillExpansionInfo( PropertyGrid::VisualState& visualState )
    {
        if ( IsExpanded() )
        {
            visualState.m_expandedPaths.emplace_back( GetPath() );
        }

        for ( auto pChildRow : m_children )
        {
            pChildRow->FillExpansionInfo( visualState );
        }
    }

    //-------------------------------------------------------------------------

    CategoryRow* CategoryRow::FindOrCreateCategory( GridRow* pParentRow, GridContext const& context, TInlineVector<CategoryRow*, 10>& categories, String const& categoryName )
    {
        int32_t insertIdx = 0;
        int32_t foundCategoryIdx = InvalidIndex;
        for ( int32_t i = 0; i < (int32_t)categories.size(); i++ )
        {
            if ( categories[i]->GetName() < categoryName )
            {
                insertIdx++;
            }

            if ( categories[i]->GetName() == categoryName )
            {
                foundCategoryIdx = i;
                break;
            }
        }

        if ( foundCategoryIdx != InvalidIndex )
        {
            return categories[foundCategoryIdx];
        }
        else
        {
            auto pNewCategory = EE::New<CategoryRow>( pParentRow, context, categoryName );
            categories.emplace( categories.begin() + insertIdx, pNewCategory );
            return pNewCategory;
        }
    }

    CategoryRow::CategoryRow( GridRow* pParentRow, GridContext const& context, String const& name )
        : GridRow( pParentRow, context, name )
    {
        if ( context.m_pRecordedVisualState != nullptr )
        {
            m_isExpanded = VectorContains( context.m_pRecordedVisualState->m_expandedCategories, m_name );
        }
    }

    void CategoryRow::DrawHeaderSection( float currentHeaderOffset )
    {
        ImGui::Dummy( ImVec2( currentHeaderOffset, 0 ) );
        ImGui::SameLine();

        ImGui::SetNextItemOpen( m_isExpanded );
        ImGui::PushStyleColor( ImGuiCol_Header, 0 );
        ImGui::PushStyleColor( ImGuiCol_HeaderActive, 0 );
        ImGui::PushStyleColor( ImGuiCol_HeaderHovered, 0 );
        m_isExpanded = ImGui::CollapsingHeader( m_name.c_str() );
        ImGui::PopStyleColor( 3 );

        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, ImGuiX::Style::s_colorGray7 );
    }

    void CategoryRow::AddProperty( IReflectedType* pTypeInstance, PropertyInfo const& propertyInfo )
    {
        EE_ASSERT( pTypeInstance != nullptr );

        GridRow* pRow = CreateRow( this, m_context, pTypeInstance, propertyInfo );

        // Sorted Insert
        //-------------------------------------------------------------------------

        size_t insertIdx = 0;
        for ( ; insertIdx < m_children.size(); insertIdx++ )
        {
            if ( m_children[insertIdx]->GetName() > pRow->GetName() )
            {
                break;
            }
        }

        m_children.insert( m_children.begin() + insertIdx, pRow );
    }

    bool CategoryRow::ShouldDrawRow() const
    {
        // Dont show empty categories
        if ( m_children.empty() )
        {
            return false;
        }

        // Always show categories if we have a visible child
        for ( auto pChildRow : m_children )
        {
            if ( pChildRow->ShouldDrawRow() )
            {
                return true;
            }
        }

        // The 'm_isHidden' flag doesnt apply to category rows, we only care about the children
        return false;
    }

    //-------------------------------------------------------------------------

    ArrayRow::ArrayRow( GridRow* pParentRow, GridContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pParentTypeInstance )
        : GridRow( pParentRow, context, propertyInfo.m_metadata.GetFriendlyName(), propertyInfo.m_metadata.HasFlag( PropertyMetadata::ReadOnly ) )
        , m_pParentTypeInstance( pParentTypeInstance )
        , m_propertyInfo( propertyInfo )
    {
        EE_ASSERT( m_propertyInfo.IsArrayProperty() );

        m_path = pParentRow->GetPath();
        m_path.Append( propertyInfo.m_ID );

        if ( context.m_pRecordedVisualState != nullptr )
        {
            m_isExpanded = VectorContains( context.m_pRecordedVisualState->m_expandedPaths, m_path );
        }

        RebuildChildren();
    }

    void ArrayRow::MoveElementUp( int32_t arrayElementIndex )
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.m_ID );

        EE_ASSERT( m_operationElementIdx == InvalidIndex );
        EE_ASSERT( arrayElementIndex > 0 && arrayElementIndex < arraySize );
        m_operationType = OperationType::MoveUp;
        m_operationElementIdx = arrayElementIndex;
    }

    void ArrayRow::MoveElementDown( int32_t arrayElementIndex )
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.m_ID );

        EE_ASSERT( m_operationElementIdx == InvalidIndex );
        EE_ASSERT( arrayElementIndex >= 0 && arrayElementIndex < ( arraySize - 1 ) );
        m_operationType = OperationType::MoveDown;
        m_operationElementIdx = arrayElementIndex;
    }

    void ArrayRow::InsertElement( int32_t insertIndex )
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.m_ID );

        EE_ASSERT( m_operationElementIdx == InvalidIndex );
        EE_ASSERT( insertIndex >= 0 && insertIndex < arraySize );
        m_operationType = OperationType::Insert;
        m_operationElementIdx = insertIndex;
    }

    void ArrayRow::DestroyElement( int32_t arrayElementIndex )
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.m_ID );

        EE_ASSERT( m_operationElementIdx == InvalidIndex );
        EE_ASSERT( arrayElementIndex >= 0 && arrayElementIndex < arraySize );
        m_operationType = OperationType::Remove;
        m_operationElementIdx = arrayElementIndex;
    }

    void ArrayRow::Update()
    {
        bool childrenNeedRebuild = false;

        // External Update Check
        //-------------------------------------------------------------------------

        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.m_ID );
        if ( m_children.size() != arraySize )
        {
            childrenNeedRebuild = true;
        }

        // Element Operations
        //-------------------------------------------------------------------------

        switch ( m_operationType )
        {
            case OperationType::Insert:
            {
                EE_ASSERT( m_operationElementIdx >= 0 && m_operationElementIdx < m_children.size() );
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::AddArrayElement );
                m_pParentTypeInstance->GetTypeInfo()->InsertArrayElement( m_pParentTypeInstance, m_propertyInfo.m_ID, m_operationElementIdx );
                childrenNeedRebuild = true;
            }
            break;

            case OperationType::MoveUp:
            {
                EE_ASSERT( m_operationElementIdx > 0 && m_operationElementIdx < m_children.size() );
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::MoveArrayElement );
                m_pParentTypeInstance->GetTypeInfo()->MoveArrayElement( m_pParentTypeInstance, m_propertyInfo.m_ID, m_operationElementIdx, m_operationElementIdx - 1 );
                childrenNeedRebuild = true;
            }
            break;

            case OperationType::MoveDown:
            {
                EE_ASSERT( m_operationElementIdx >= 0 && m_operationElementIdx < ( m_children.size() - 1 ) );
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::MoveArrayElement );
                m_pParentTypeInstance->GetTypeInfo()->MoveArrayElement( m_pParentTypeInstance, m_propertyInfo.m_ID, m_operationElementIdx, m_operationElementIdx + 1 );
                childrenNeedRebuild = true;
            }
            break;

            case OperationType::Remove:
            {
                EE_ASSERT( m_operationElementIdx >= 0 && m_operationElementIdx < m_children.size() );
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::RemoveArrayElement );
                m_pParentTypeInstance->GetTypeInfo()->RemoveArrayElement( m_pParentTypeInstance, m_propertyInfo.m_ID, m_operationElementIdx );
                childrenNeedRebuild = true;
            }
            break;

            case OperationType::PushBack:
            {
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::AddArrayElement );
                m_pParentTypeInstance->GetTypeInfo()->AddArrayElement( m_pParentTypeInstance, m_propertyInfo.m_ID );
                childrenNeedRebuild = true;
                m_isExpanded = true; // Expand the array to show the new element
            }
            break;

            case OperationType::RemoveAll:
            {
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::RemoveArrayElement );
                m_pParentTypeInstance->GetTypeInfo()->ClearArray( m_pParentTypeInstance, m_propertyInfo.m_ID );
                childrenNeedRebuild = true;
            }
            break;

            case OperationType::Reset:
            {
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo );
                m_pParentTypeInstance->GetTypeInfo()->ResetToDefault( m_pParentTypeInstance, m_propertyInfo.m_ID );
                childrenNeedRebuild = true;
            }
            break;

            default:
            break;
        }

        if ( childrenNeedRebuild )
        {
            PropertyGrid::VisualState visualState;
            FillExpansionInfo( visualState );
            m_context.m_pRecordedVisualState = &visualState;
            RebuildChildren();
            m_context.m_pRecordedVisualState = nullptr;
        }

        //-------------------------------------------------------------------------

        // No operation last more than a frame
        m_operationType = OperationType::None;
        m_operationElementIdx = InvalidIndex;

        // Visibility
        //-------------------------------------------------------------------------
        // Array element visibility is tied to our visibility!

        if ( !m_isHidden )
        {
            for ( auto pChild : m_children )
            {
                pChild->SetHidden( false );
            }
        }
    }

    bool ArrayRow::ShouldDrawRow() const
    {
        for ( auto pChildRow : m_children )
        {
            if ( pChildRow->ShouldDrawRow() )
            {
                return true;
            }
        }

        return !m_isHidden;
    }

    void ArrayRow::DrawHeaderSection( float currentHeaderOffset )
    {
        ImGui::Dummy( ImVec2( currentHeaderOffset, 0 ) );
        ImGui::SameLine();

        ImGui::SetNextItemOpen( m_isExpanded );
        ImGui::PushStyleColor( ImGuiCol_Header, 0 );
        ImGui::PushStyleColor( ImGuiCol_HeaderActive, 0 );
        ImGui::PushStyleColor( ImGuiCol_HeaderHovered, 0 );
        m_isExpanded = ImGui::CollapsingHeader( m_name.c_str() );
        ImGui::PopStyleColor( 3 );

        if ( !m_propertyInfo.m_metadata.GetDescription().empty() )
        {
            ImGuiX::ItemTooltip( m_propertyInfo.m_metadata.GetDescription().c_str() );
        }
    }

    void ArrayRow::DrawEditorSection()
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.m_ID );
        ImGui::TextColored( Colors::Gray.ToFloat4(), "%d Elements", arraySize );
    }

    bool ArrayRow::HasExtraControls() const
    {
        return m_propertyInfo.IsDynamicArrayProperty() && !m_propertyInfo.m_metadata.HasFlag( PropertyMetadata::ShowAsStaticArray );
    }

    float ArrayRow::GetExtraControlsSectionWidth() const
    {
        return ( g_controlButtonSize.x * 2 ) + 4;
    }

    void ArrayRow::DrawExtraControlsSection()
    {
        ImGui::BeginDisabled( IsReadOnly() );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        if ( ImGuiX::FlatButton( EE_ICON_PLUS, g_controlButtonSize, Colors::LightGreen) )
        {
            m_operationType = OperationType::PushBack;
            m_operationElementIdx = InvalidIndex;
        }
        ImGui::PopStyleVar();
        ImGuiX::ItemTooltip( "Add array element" );

        ImGui::SameLine();
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        if ( ImGuiX::FlatButton( EE_ICON_TRASH_CAN, g_controlButtonSize, Colors::PaleVioletRed ) )
        {
            m_operationType = OperationType::RemoveAll;
            m_operationElementIdx = InvalidIndex;
        }
        ImGui::PopStyleVar();
        ImGuiX::ItemTooltip( "Remove all array elements" );

        ImGui::EndDisabled();
    }

    bool ArrayRow::HasResetSection() const
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        return !pTypeInfo->IsPropertyValueSetToDefault( m_pParentTypeInstance, m_propertyInfo.m_ID ) && !m_propertyInfo.m_metadata.HasFlag( PropertyMetadata::ShowAsStaticArray );
    }

    void ArrayRow::DrawResetSection()
    {
        ImGui::BeginDisabled( IsReadOnly() );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        if ( ImGuiX::FlatButton( EE_ICON_UNDO_VARIANT, g_controlButtonSize, Colors::LightGray) )
        {
            m_operationType = OperationType::Reset;
            m_operationElementIdx = InvalidIndex;
        }
        ImGui::PopStyleVar();
        ImGuiX::ItemTooltip( "Reset value to default" );

        ImGui::EndDisabled();
    }

    void ArrayRow::RebuildChildren()
    {
        DestroyChildren();

        //-------------------------------------------------------------------------

        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.m_ID );
        for ( auto i = 0u; i < arraySize; i++ )
        {
            m_children.emplace_back( CreateRow( this, m_context, m_pParentTypeInstance, m_propertyInfo, i ) );
        }
    }

    //-------------------------------------------------------------------------

    PropertyRow::PropertyRow( GridRow* pParentRow, GridContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pParentTypeInstance, int32_t arrayElementIndex )
        : GridRow( pParentRow, context, propertyInfo.m_metadata.GetFriendlyName(), propertyInfo.m_metadata.HasFlag( PropertyMetadata::ReadOnly ), propertyInfo.m_metadata.HasFlag( PropertyMetadata::Hidden ) )
        , m_propertyInfo( propertyInfo )
        , m_pParentTypeInstance( pParentTypeInstance )
        , m_arrayElementIdx( arrayElementIndex )
    {
        EE_ASSERT( m_pParentTypeInstance != nullptr );
        EE_ASSERT( propertyInfo.IsValid() );

        m_path = pParentRow->GetPath();
        m_path.Append( propertyInfo.m_ID, arrayElementIndex );

        //-------------------------------------------------------------------------

        if ( context.m_pRecordedVisualState != nullptr )
        {
            m_isExpanded = VectorContains( context.m_pRecordedVisualState->m_expandedPaths, m_path );
        }

        //-------------------------------------------------------------------------

        if ( arrayElementIndex != InvalidIndex )
        {
            m_name = String( String::CtorSprintf(), "%d", m_arrayElementIdx );
            m_pPropertyInstance = m_pParentTypeInstance->GetTypeInfo()->GetArrayElementDataPtr( m_pParentTypeInstance, propertyInfo.m_ID, m_arrayElementIdx );
        }
        else
        {
            m_pPropertyInstance = reinterpret_cast<uint8_t*>( m_pParentTypeInstance ) + propertyInfo.m_offset;
        }

        //-------------------------------------------------------------------------

        m_pPropertyEditor = PropertyGridEditorFactory::TryCreateEditor( m_context, m_propertyInfo, m_pParentTypeInstance, m_pPropertyInstance );
        m_pTypeEditingRules = TypeEditingRulesFactory::TryCreateRules( m_context.m_pToolsContext, m_pParentTypeInstance );

        //-------------------------------------------------------------------------

        RebuildChildren();
        UpdateName();
    }

    PropertyRow::~PropertyRow()
    {
        EE::Delete( m_pPropertyEditor );
        EE::Delete( m_pTypeEditingRules );
    }

    bool PropertyRow::ShouldDrawRow() const
    {
        if ( m_propertyInfo.IsStructureProperty() && !HasPropertyEditor() )
        {
            for ( auto pChildRow : m_children )
            {
                if ( pChildRow->ShouldDrawRow() )
                {
                    return true;
                }
            }

            return false;
        }
        else
        {
            return !m_isHidden;
        }
    }

    void PropertyRow::Update()
    {
        bool childrenNeedRebuild = false;

        switch ( m_operationType )
        {
            case OperationType::Update:
            {
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo );
                m_pPropertyEditor->UpdatePropertyValue();
                childrenNeedRebuild = true;
            }
            break;

            case OperationType::Reset:
            {
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, this, m_pParentTypeInstance, &m_propertyInfo );
                m_pParentTypeInstance->GetTypeInfo()->ResetToDefault( m_pParentTypeInstance, m_propertyInfo.m_ID );

                if ( HasPropertyEditor() )
                {
                    m_pPropertyEditor->ResetWorkingCopy();

                    if ( m_propertyInfo.IsTypeInstanceProperty() )
                    {
                        childrenNeedRebuild = true;
                    }
                }
                else
                {
                    childrenNeedRebuild = true;
                }
            }
            break;

            default:
            break;
        }

        if ( childrenNeedRebuild )
        {
            PropertyGrid::VisualState visualState;
            FillExpansionInfo( visualState );
            m_context.m_pRecordedVisualState = &visualState;
            RebuildChildren();
            m_context.m_pRecordedVisualState = nullptr;
        }

        //-------------------------------------------------------------------------

        // No operation last more than a frame
        m_operationType = OperationType::None;

        //-------------------------------------------------------------------------

        if ( m_pTypeEditingRules != nullptr )
        {
            // Only evaluate extra rules for mutable properties
            if ( !IsReadOnly() )
            {
                m_isReadOnly = m_pTypeEditingRules->IsReadOnly( m_propertyInfo.m_ID ) == TypeEditingRules::ReadOnlyState::ReadOnly;
            }

            // Only evaluate extra rules for visible properties
            if ( !IsHidden() )
            {
                m_isHidden = m_pTypeEditingRules->IsHidden( m_propertyInfo.m_ID ) == TypeEditingRules::HiddenState::Hidden;
            }
        }
    }

    void PropertyRow::UpdateName()
    {
        // Type editing rules will take precedence
        if ( m_pTypeEditingRules != nullptr )
        {
            InlineString const nameOverride = m_pTypeEditingRules->GetPropertyNameOverride( m_propertyInfo.m_ID, m_arrayElementIdx );
            if ( !nameOverride.empty() )
            {
                m_name = nameOverride;
            }
        }
        else if ( m_pPropertyEditor != nullptr )
        {
            InlineString const nameOverride = m_pPropertyEditor->GetPropertyNameOverride( m_arrayElementIdx );
            if ( !nameOverride.empty() )
            {
                m_name = nameOverride;
            }
        }
    }

    void PropertyRow::DrawHeaderSection( float currentHeaderOffset )
    {
        ImGui::Dummy( ImVec2( currentHeaderOffset, 0 ) );
        ImGui::SameLine();

        //-------------------------------------------------------------------------

        if ( m_propertyInfo.IsTypeInstanceProperty() )
        {
            TypeInstance const* pTypeInstanceContainer = reinterpret_cast<TypeInstance*>( m_pPropertyInstance );

            ImGui::SetNextItemOpen( m_isExpanded );
            ImGui::PushStyleColor( ImGuiCol_Header, 0 );
            ImGui::PushStyleColor( ImGuiCol_HeaderActive, 0 );
            ImGui::PushStyleColor( ImGuiCol_HeaderHovered, 0 );
            m_isExpanded = ImGui::CollapsingHeader( m_name.c_str(), pTypeInstanceContainer->IsSet() ? 0 : ImGuiTreeNodeFlags_Leaf );
            ImGui::PopStyleColor( 3 );

            if ( !m_propertyInfo.m_metadata.GetDescription().empty() )
            {
                ImGuiX::ItemTooltip( m_propertyInfo.m_metadata.GetDescription().c_str() );
            }
        }
        else if ( HasPropertyEditor() )
        {
            ImGui::Text( m_name.c_str() );

            if ( !m_propertyInfo.m_metadata.GetDescription().empty() )
            {
                ImGuiX::TextTooltip( m_propertyInfo.m_metadata.GetDescription().c_str() );
            }
            else
            {
                ImGuiX::TextTooltip( m_name.c_str() );
            }
        }
        else if ( m_propertyInfo.IsStructureProperty() )
        {
            ImGui::SetNextItemOpen( m_isExpanded );
            ImGui::PushStyleColor( ImGuiCol_Header, 0 );
            ImGui::PushStyleColor( ImGuiCol_HeaderActive, 0 );
            ImGui::PushStyleColor( ImGuiCol_HeaderHovered, 0 );
            m_isExpanded = ImGui::CollapsingHeader( m_name.c_str() );
            ImGui::PopStyleColor( 3 );

            if ( !m_propertyInfo.m_metadata.GetDescription().empty() )
            {
                ImGuiX::ItemTooltip( m_propertyInfo.m_metadata.GetDescription().c_str() );
            }
        }
        else
        {
            ImGui::Text( m_name.c_str() );
        }
    }

    void PropertyRow::DrawEditorSection()
    {
        if ( HasPropertyEditor() )
        {
            ImGui::BeginDisabled( IsReadOnly() );

            PropertyEditor::Result const result = m_pPropertyEditor->UpdateAndDraw();
            if ( result == PropertyEditor::Result::ValueUpdated )
            {
                m_operationType = OperationType::Update;
            }
            else if ( result == PropertyEditor::Result::ValueUpdatedAndGridNeedsRebuild )
            {
                EE_ASSERT( m_context.m_pRowThatRequiresUpdateAndRebuild == nullptr );
                m_operationType = OperationType::Update;
                m_context.m_pRowThatRequiresUpdateAndRebuild = this;
            }
            ImGui::EndDisabled();
        }
        else if ( m_propertyInfo.IsStructureProperty() )
        {
            auto pTypeInfo = reinterpret_cast<IReflectedType*>( m_pPropertyInstance )->GetTypeInfo();
            InlineString const structTypeName( InlineString::CtorSprintf(), "Type: [%s]", pTypeInfo->m_ID.c_str() );
            ImGui::TextColored( Colors::Gray.ToFloat4(), structTypeName.c_str() );
        }
        else
        {
            ImGui::Text( EE_ICON_EXCLAMATION" Missing Editor" );
        }
    }

    bool PropertyRow::HasExtraControls() const
    {
        if ( m_propertyInfo.IsStaticArrayProperty() )
        {
            return false;
        }

        bool const isDynamicArrayElement = ( m_arrayElementIdx != InvalidIndex );
        if ( isDynamicArrayElement )
        {
            return !m_propertyInfo.m_metadata.HasFlag( PropertyMetadata::ShowAsStaticArray );
        }

        return false;
    }

    float PropertyRow::GetExtraControlsSectionWidth() const
    {
        return g_controlButtonSize.x;
    }

    void PropertyRow::DrawExtraControlsSection()
    {
        // Draw array element options
        //-------------------------------------------------------------------------

        if ( m_propertyInfo.IsArrayProperty() )
        {
            EE_ASSERT( m_arrayElementIdx >= 0 );

            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
            ImGuiX::FlatButton( EE_ICON_DOTS_HORIZONTAL, g_controlButtonSize, ImGuiX::Style::s_colorText );
            ImGui::PopStyleVar();

            if ( ImGui::BeginPopupContextItem( 0, ImGuiPopupFlags_MouseButtonLeft ) )
            {
                if ( ImGui::MenuItem( EE_ICON_PLUS" Insert New Element" ) )
                {
                    static_cast<ArrayRow*>( m_pParent )->InsertElement( m_arrayElementIdx );
                }

                if ( m_arrayElementIdx > 0 )
                {
                    if ( ImGui::MenuItem( EE_ICON_ARROW_UP" Move Element Up" ) )
                    {
                        static_cast<ArrayRow*>( m_pParent )->MoveElementUp( m_arrayElementIdx );
                    }
                }

                auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
                size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.m_ID );
                if ( m_arrayElementIdx < ( arraySize - 1 ) )
                {
                    if ( ImGui::MenuItem( EE_ICON_ARROW_DOWN" Move Element Down" ) )
                    {
                        static_cast<ArrayRow*>( m_pParent )->MoveElementDown( m_arrayElementIdx );
                    }
                }

                if ( ImGui::MenuItem( EE_ICON_TRASH_CAN" Remove Element" ) )
                {
                    static_cast<ArrayRow*>( m_pParent )->DestroyElement( m_arrayElementIdx );
                }
                ImGui::EndPopup();
            }

            ImGuiX::ItemTooltip( "Array Element Options" );
        }
    }

    bool PropertyRow::HasResetSection() const
    {
        // If this is a code-only type instance, we dont allow resetting of the created instance
        if ( m_propertyInfo.IsTypeInstanceProperty() )
        {
            bool const isTypePickerDisabled = m_propertyInfo.m_metadata.HasFlag( PropertyMetadata::DisableTypePicker );
            if ( isTypePickerDisabled )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        return !pTypeInfo->IsPropertyValueSetToDefault( m_pParentTypeInstance, m_propertyInfo.m_ID, m_arrayElementIdx );
    }

    void PropertyRow::DrawResetSection()
    {
        ImGui::BeginDisabled( IsReadOnly() );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        if ( ImGuiX::FlatButton( EE_ICON_UNDO_VARIANT, g_controlButtonSize, Colors::LightGray ) )
        {
            m_operationType = OperationType::Reset;
        }
        ImGui::PopStyleVar();
        ImGuiX::ItemTooltip( "Reset value to default" );
        ImGui::EndDisabled();
    }

    void PropertyRow::RebuildChildren()
    {
        DestroyChildren();

        //-------------------------------------------------------------------------

        if ( HasPropertyEditor() && !m_propertyInfo.IsTypeInstanceProperty() )
        {
            return;
        }

        if ( !m_propertyInfo.IsStructureProperty() && !m_propertyInfo.IsTypeInstanceProperty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        IReflectedType* pChildTypeInstance = nullptr;

        if ( m_propertyInfo.IsTypeInstanceProperty() )
        {
            TypeInstance* pTypeInstanceContainer = reinterpret_cast<TypeInstance*>( m_pPropertyInstance );
            if ( pTypeInstanceContainer->IsSet() )
            {
                pChildTypeInstance = pTypeInstanceContainer->Get();
            }
        }
        else if ( m_propertyInfo.IsStructureProperty() )
        {
            pChildTypeInstance = reinterpret_cast<IReflectedType*>( m_pPropertyInstance );
        }

        //-------------------------------------------------------------------------

        if ( pChildTypeInstance != nullptr )
        {
            TInlineVector<CategoryRow*, 10> categories;

            TypeInfo const* pTypeInfo = pChildTypeInstance->GetTypeInfo();
            for ( auto const& propertyInfo : pTypeInfo->m_properties )
            {
                if ( propertyInfo.m_metadata.HasFlag( PropertyMetadata::Flag::Hidden ) )
                {
                    continue;
                }

                String const category = propertyInfo.m_metadata.GetValue( PropertyMetadata::Flag::Category, "General" );
                PG::CategoryRow* pCategory = PG::CategoryRow::FindOrCreateCategory( this, m_context, categories, category );
                pCategory->AddProperty( pChildTypeInstance, propertyInfo );
            }

            //-------------------------------------------------------------------------

            // If we only have a single category, there's no point polluting the grid with it
            if ( categories.size() == 1 )
            {
                for ( auto pProperty : categories[0]->GetChildren() )
                {
                    pProperty->SetParent( this );
                    m_children.emplace_back( pProperty );
                }

                EE::Delete( categories[0] );
            }
            else // Add all categories
            {
                for ( auto pCategory : categories )
                {
                    m_children.emplace_back( pCategory );
                }
            }
        }
    }

    void PropertyRow::GeneratePropertyChangedNotificationChain( TVector<PropertyChainElement>& outChain ) const
    {
        if ( m_pParent != nullptr )
        {
            m_pParent->GeneratePropertyChangedNotificationChain( outChain );
        }

        outChain.emplace_back( m_pParentTypeInstance, &m_propertyInfo );
    }
}