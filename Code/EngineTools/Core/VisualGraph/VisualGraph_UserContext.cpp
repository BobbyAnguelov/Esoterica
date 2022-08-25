#include "VisualGraph_UserContext.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    void UserContext::NavigateTo( VisualGraph::BaseNode* pNode )
    {
        EE_ASSERT( pNode != nullptr );
        m_onNavigateToNode.Execute( pNode );
    }

    void UserContext::NavigateTo( VisualGraph::BaseGraph* pGraph )
    {
        EE_ASSERT( pGraph != nullptr );
        m_onNavigateToGraph.Execute( pGraph );
    }
}