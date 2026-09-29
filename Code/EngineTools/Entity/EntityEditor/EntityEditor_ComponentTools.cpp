#include "EntityEditor_ComponentTools.h"
#include "EntityEditor_Context.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    ComponentTools* ComponentToolsFactory::TryCreateTools( EditorContext &context, EntityComponent* pComponent )
    {
        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pComponent->GetTypeInfo()->IsDerivedFrom( pCurrentFactory->GetSupportedTypeID() ) )
            {
                return pCurrentFactory->TryCreateInternal( context, pComponent );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return nullptr;
    }

    void ComponentToolsFactory::ForEachFactory( TFunction<void( ComponentToolsFactory const* )>&& fn )
    {
        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            fn( pCurrentFactory );
            pCurrentFactory = pCurrentFactory->GetNextItem();
        }
    }

    //-------------------------------------------------------------------------

    ComponentToolsManager::ViewportWidgetLists::ViewportWidgetLists( ComponentTools const* pComponentTools )
        : m_pComponentTools( pComponentTools )
        , m_pRequiredComponentTypeInfo( pComponentTools->GetRequiredComponentType() )
    {}

    void ComponentToolsManager::Initialize( EditorContext &context )
    {
        EE_ASSERT( m_pContext == nullptr );
        m_pContext = &context;
        m_postWorldChangedEventID = m_pContext->OnPostWorldChange().Bind( [this] () { DestroyCreatedComponentTools(); } );

        //-------------------------------------------------------------------------

        auto Fn = [this] ( ComponentToolsFactory const* pFactory )
        {
            m_defaultToolInstances.emplace_back( pFactory->CreateDefaultInstance() );
        };

        ComponentToolsFactory::ForEachFactory( Fn );

        //-------------------------------------------------------------------------

        for ( auto pDefaultToolsInstance : m_defaultToolInstances )
        {
            if ( pDefaultToolsInstance->HasViewportSelectionWidget() )
            {
                m_viewportWidgetLists.emplace_back( pDefaultToolsInstance );
            }
        }
    }

    void ComponentToolsManager::Shutdown()
    {
        m_viewportWidgetLists.clear();

        for ( auto& pDefaultToolsInstance : m_defaultToolInstances )
        {
            EE::Delete( pDefaultToolsInstance );
        }

        //-------------------------------------------------------------------------

        DestroyCreatedComponentTools();
        m_pContext->OnPostWorldChange().Unbind( m_postWorldChangedEventID );
        m_pContext = nullptr;
    }

    void ComponentToolsManager::DestroyCreatedComponentTools()
    {
        for ( auto pEditor : m_componentTools )
        {
            EE::Delete( pEditor );
        }
        m_componentTools.clear();
    }

    void ComponentToolsManager::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
        {
            for ( auto& viewportWidgetList : m_viewportWidgetLists )
            {
                if ( pSpatialComponent->GetTypeInfo()->IsDerivedFrom( viewportWidgetList.m_pRequiredComponentTypeInfo->m_ID ) )
                {
                    viewportWidgetList.m_entityItems.emplace_back( pEntity, pSpatialComponent );
                }
            }
        }
    }

    void ComponentToolsManager::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
        {
            for ( auto& viewportWidgetList : m_viewportWidgetLists )
            {
                if ( pSpatialComponent->GetTypeInfo()->IsDerivedFrom( viewportWidgetList.m_pRequiredComponentTypeInfo->m_ID ) )
                {
                    viewportWidgetList.m_entityItems.erase_first_unsorted( EntityEditorItem( pEntity, pSpatialComponent ) );
                }
            }
        }
    }

    void ComponentToolsManager::Update()
    {
        EE_ASSERT( IsInitialized() );

        // Do not show editors for multi-entity selections
        int32_t numSelectedEntities = 0;
        for ( auto const& selectedItem : m_pContext->GetSelection() )
        {
            if ( selectedItem.IsEntity() )
            {
                numSelectedEntities++;
            }
        }

        if ( numSelectedEntities > 1 )
        {
            DestroyCreatedComponentTools();
            return;
        }

        //-------------------------------------------------------------------------

        TInlineVector<EntityComponent*, 30> selectedComponents;
        for ( auto const& selectedItem : m_pContext->GetSelection() )
        {
            if ( selectedItem.IsEntity() )
            {
                for ( auto pComponent : selectedItem.m_pEntity->GetComponents() )
                {
                    VectorEmplaceBackUnique( selectedComponents, pComponent );
                }
            }

            if ( selectedItem.IsComponent() )
            {
                VectorEmplaceBackUnique( selectedComponents, selectedItem.m_pComponent );
            }
        }

        // Compare created editors vs selected components
        //-------------------------------------------------------------------------

        for ( int32_t i = int32_t( m_componentTools.size() ) - 1; i >= 0; i-- )
        {
            auto pEditedComponent = m_componentTools[i]->GetEditedComponent();
            int32_t const selectedComponentIdx = VectorFindIndex( selectedComponents, pEditedComponent );
            if ( selectedComponentIdx != InvalidIndex )
            {
                selectedComponents.erase_unsorted( selectedComponents.begin() + selectedComponentIdx );
            }
            else
            {
                EE::Delete( m_componentTools[i] );
                m_componentTools.erase_unsorted( m_componentTools.begin() + i );
            }
        }

        // Create any missing editors
        //-------------------------------------------------------------------------

        for ( auto pSelectedComponent : selectedComponents )
        {
            auto pCreatedEditor = ComponentToolsFactory::TryCreateTools( *m_pContext, pSelectedComponent );
            if ( pCreatedEditor != nullptr )
            {
                m_componentTools.emplace_back( pCreatedEditor );
            }
        }
    }

    void ComponentToolsManager::DrawViewportEditors( Viewport const* pViewport, bool isFocused )
    {
        EE_ASSERT( IsInitialized() );

        m_isManipulating = false;

        for ( auto pComponentTools : m_componentTools )
        {
            m_isManipulating |= pComponentTools->DrawViewportEditor( *m_pContext, pViewport, isFocused );
        }
    }

    void ComponentToolsManager::DrawViewportWidgets( Viewport const* pViewport, bool isFocused )
    {
        EE_ASSERT( IsInitialized() );

        ImGuiX::ScopedFont sf( ImGuiX::FontType::Regular, 48, Colors::Red );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 0 ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Colors::Transparent );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, Colors::Transparent );

        for ( auto& viewportWidgetList : m_viewportWidgetLists )
        {
            ImVec2 const buttonSize = ImGuiX::CalculateButtonDimensions( viewportWidgetList.m_pComponentTools->GetViewportSelectionWidgetIcon() );
            InlineString const buttonLabel( InlineString::CtorSprintf(), "%s##SB", viewportWidgetList.m_pComponentTools->GetViewportSelectionWidgetIcon() );
            Color const iconDefaultColor = viewportWidgetList.m_pComponentTools->GetViewportSelectionWidgetColor();

            for ( auto const& item : viewportWidgetList.m_entityItems )
            {
                auto pSpatialComponent = item.GetSpatialComponent();

                if ( !pViewport->GetViewVolume().Contains( pSpatialComponent->GetWorldBounds().GetAABB() ) )
                {
                    continue;
                }

                auto const positionSS = pViewport->WorldSpaceToScreenSpace( pSpatialComponent->GetWorldTransform().GetTranslation() );
                ImVec2 const imguiPos = ImVec2( positionSS ) - ImGui::GetWindowPos() - ( buttonSize / 2 );
                ImRect const buttonRect( imguiPos, imguiPos + buttonSize );

                Color color = iconDefaultColor; 
                if ( buttonRect.Contains( ImGui::GetMousePos() - ImGui::GetWindowPos() ) )
                {
                    color = iconDefaultColor.GetScaledColor( 0.85f );
                }

                ImGui::PushStyleColor( ImGuiCol_Text, color );
                ImGui::PushID( pSpatialComponent );

                ImGui::SetCursorPos( imguiPos );
                if ( ImGuiX::FlatButton( buttonLabel.c_str(), buttonSize ) )
                {
                    if ( ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl )
                    {
                        m_pContext->AddToSelection( item );
                    }
                    else
                    {
                        m_pContext->SetSelection( item );
                    }
                }

                ImGui::PopID();
                ImGui::PopStyleColor();
            }
        }

        ImGui::PopStyleColor( 2 );
        ImGui::PopStyleVar();
    }
}