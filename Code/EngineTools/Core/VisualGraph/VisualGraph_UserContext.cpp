#include "VisualGraph_UserContext.h"
#include "VisualGraph_BaseGraph.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    SelectedNode::SelectedNode( BaseNode* pNode )
        : m_nodeID( pNode->GetID() )
        , m_pNode( pNode )
    {}

    bool SelectedNode::operator==( SelectedNode const& rhs ) const
    {
        return m_nodeID == rhs.m_nodeID;
    }

    bool SelectedNode::operator==( BaseNode const* pNode ) const
    {
        return m_nodeID == pNode->GetID();
    }

    //-------------------------------------------------------------------------

    void UserContext::NavigateTo( BaseNode* pNode )
    {
        EE_ASSERT( pNode != nullptr );
        m_navigateToNodeEvent.Execute( pNode );
    }

    void UserContext::DoubleClick( BaseNode* pNode )
    {
        EE_ASSERT( pNode != nullptr );
        m_nodeDoubleClickedEvent.Execute( pNode );
    }

    void UserContext::NavigateTo( BaseGraph* pGraph )
    {
        EE_ASSERT( pGraph != nullptr );
        m_navigateToGraphEvent.Execute( pGraph );
    }

    void UserContext::DoubleClick( BaseGraph* pGraph )
    {
        EE_ASSERT( pGraph != nullptr );
        m_graphDoubleClickedEvent.Execute( pGraph );
    }

    void UserContext::RequestOpenResource( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        m_requestOpenResourceEvent.Execute( resourceID );
    }

    void UserContext::NotifySelectionChanged( TVector<SelectedNode> const& oldSelection, TVector<SelectedNode> const& newSelection )
    {
        m_selectionChangedEvent.Execute( oldSelection, newSelection );
    }

    void UserContext::NotifyNodesPasted( TInlineVector<BaseNode*, 20> const& pastedNodes )
    {
        m_postPasteEvent.Execute( pastedNodes );
    }

    void UserContext::ResetExtraTitleInfoTextColor()
    {
        m_extraTitleInfoColor = ImGuiX::Style::s_colorText.operator ImU32();
    }

    void UserContext::RequestAdvancedCommand( TSharedPtr<AdvancedCommand> const& command )
    {
        m_advancedCommandRequestedEvent.Execute( command );
    }
}