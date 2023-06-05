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

    //-------------------------------------------------------------------------

    PoseNodeDebugInfo ToolsGraphUserContext::GetPoseNodeDebugInfo( int16_t runtimeNodeIdx ) const
    {
        return m_pGraphInstance->GetPoseNodeDebugInfo( runtimeNodeIdx );
    }
}