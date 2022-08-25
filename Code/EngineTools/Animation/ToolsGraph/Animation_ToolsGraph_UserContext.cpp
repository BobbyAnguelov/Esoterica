#include "Animation_ToolsGraph_UserContext.h"
#include "Nodes/Animation_ToolsGraphNode.h"
#include "System/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ToolsGraphUserContext::RequestOpenResource( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        m_onRequestOpenResource.Execute( resourceID );
    }

    bool ToolsGraphUserContext::IsNodeActive( int16_t nodeIdx ) const
    {
        return m_pGraphInstance->IsNodeActive( nodeIdx );
    }

    #if EE_DEVELOPMENT_TOOLS
    PoseNodeDebugInfo ToolsGraphUserContext::GetPoseNodeDebugInfo( int16_t runtimeNodeIdx ) const
    {
        return m_pGraphInstance->GetPoseNodeDebugInfo( runtimeNodeIdx );
    }
    #endif
}