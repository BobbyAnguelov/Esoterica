#include "Animation_ToolsGraph_UserContext.h"
#include "Nodes/Animation_ToolsGraphNode.h"
#include "System/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    bool ToolsGraphUserContext::IsNodeActive( int16_t nodeIdx ) const
    {
        return m_pGraphInstance->IsNodeActive( nodeIdx );
    }

    TVector<int16_t> const& ToolsGraphUserContext::GetActiveNodes() const
    {
        return m_pGraphInstance->GetActiveNodes();
    }

    void ToolsGraphUserContext::OpenChildGraph( VisualGraph::BaseNode* pSourceNode, ResourceID const& graphID, bool openInNewWorkspace )
    {
        EE_ASSERT( graphID.IsValid() );
        m_navigateToChildGraphEvent.Execute( pSourceNode, graphID, openInNewWorkspace );
    }

    //-------------------------------------------------------------------------

    PoseNodeDebugInfo ToolsGraphUserContext::GetPoseNodeDebugInfo( int16_t runtimeNodeIdx ) const
    {
        return m_pGraphInstance->GetPoseNodeDebugInfo( runtimeNodeIdx );
    }
}