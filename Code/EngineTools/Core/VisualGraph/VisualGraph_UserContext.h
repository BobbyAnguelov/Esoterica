#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Types/Event.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Memory/Pointers.h"
#include "Base/Types/StringID.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    class BaseNode;
    class BaseGraph;

    //-------------------------------------------------------------------------

    // Helper to unsure we can maintain selection after a undo/redo
    struct EE_ENGINETOOLS_API SelectedNode
    {
        SelectedNode() = default;
        SelectedNode( BaseNode* pNode );
        bool operator==( SelectedNode const& rhs ) const;
        bool operator==( BaseNode const* pNode ) const;

        UUID            m_nodeID;
        BaseNode*       m_pNode = nullptr;
    };

    //-------------------------------------------------------------------------

    struct AdvancedCommand : public IReflectedType
    {
        EE_REFLECT_TYPE( AdvancedCommand );

    public:

        VisualGraph::BaseNode* m_pCommandSourceNode = nullptr;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API UserContext
    {
        // Extra graph title info
        //-------------------------------------------------------------------------

        String const& GetExtraGraphTitleInfoText() const { return m_extraGraphTitleInfo; }
        void SetExtraGraphTitleInfoText( String const& newInfoText ) { m_extraGraphTitleInfo = newInfoText; }
        void SetExtraGraphTitleInfoText( char const* pText ) { m_extraGraphTitleInfo = pText; }
        void ClearExtraGraphTitleInfoText() { m_extraGraphTitleInfo.clear(); }

        uint32_t GetExtraTitleInfoTextColor() const { return m_extraTitleInfoColor; }
        void SetExtraTitleInfoTextColor( uint32_t newColor ) { m_extraTitleInfoColor = newColor; }
        void ResetExtraTitleInfoTextColor();

        void ResetExtraTitleInfo() { ClearExtraGraphTitleInfoText(); ResetExtraTitleInfoTextColor(); }

        // Node and graph events
        //-------------------------------------------------------------------------

        void NavigateTo( BaseNode* pNode );
        void NavigateTo( BaseGraph* pGraph );
        void DoubleClick( BaseNode* pNode );
        void DoubleClick( BaseGraph* pGraph );
        void RequestAdvancedCommand( TSharedPtr<AdvancedCommand> const& command );

        inline TEventHandle<BaseNode*> OnNavigateToNode() { return m_navigateToNodeEvent; }
        inline TEventHandle<BaseNode*> OnNodeDoubleClicked() { return m_nodeDoubleClickedEvent; }
        inline TEventHandle<BaseGraph*> OnNavigateToGraph() { return m_navigateToGraphEvent; }
        inline TEventHandle<BaseGraph*> OnGraphDoubleClicked() { return m_graphDoubleClickedEvent; }
        inline TEventHandle<TSharedPtr<AdvancedCommand>> OnAdvancedCommandRequested() { return m_advancedCommandRequestedEvent; }

        // Misc events
        //-------------------------------------------------------------------------

        void RequestOpenResource( ResourceID const& resourceID );

        void NotifySelectionChanged( TVector<SelectedNode> const& oldSelection, TVector<SelectedNode> const& newSelection );

        void NotifyNodesPasted( TInlineVector<BaseNode*, 20> const& pastedNodes );

        // Fired whenever we received an open resource request
        inline TEventHandle<ResourceID const&> OnRequestOpenResource() { return m_requestOpenResourceEvent; }

        // Event fired whenever the selection changes. First arg is the old selection, second arg is the new selection
        inline TEventHandle<TVector<SelectedNode> const&, TVector<SelectedNode> const&> OnSelectionChanged() { return m_selectionChangedEvent; }

        // Event fired whenever we paste nodes but before we complete the graph modification, allows for addition validation, manipulation of data
        inline TEventHandle<TInlineVector<BaseNode*, 20> const&> OnPostPasteNodes() { return m_postPasteEvent; }

    public:

        bool                                                                    m_isAltDown = false;
        bool                                                                    m_isCtrlDown = false;
        bool                                                                    m_isShiftDown = false;

    protected:

        TEvent<BaseNode*>                                                       m_navigateToNodeEvent;
        TEvent<BaseNode*>                                                       m_nodeDoubleClickedEvent;
        TEvent<BaseGraph*>                                                      m_navigateToGraphEvent;
        TEvent<BaseGraph*>                                                      m_graphDoubleClickedEvent;
        TEvent<ResourceID const&>                                               m_requestOpenResourceEvent;
        TEvent<TSharedPtr<AdvancedCommand>>                                     m_advancedCommandRequestedEvent;
        TEvent<TInlineVector<BaseNode*, 20> const&>                             m_postPasteEvent;
        TEvent<TVector<SelectedNode> const&, TVector<SelectedNode> const&>      m_selectionChangedEvent;

        String                                                                  m_extraGraphTitleInfo;
        uint32_t                                                                m_extraTitleInfoColor;
    };
}