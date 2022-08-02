#include "Animation_EditorGraphNode_ExternalGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_ExternalGraph.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ExternalGraphEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateOutputPin( "Pose", GraphValueType::Pose );
        m_name = GetUniqueSlotName( "External Graph" );
    }

    int16_t ExternalGraphEditorNode::Compile( GraphCompilationContext& context ) const
    {
        ExternalGraphNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ExternalGraphNode>( this, pSettings );
        context.RegisterExternalGraphSlotNode( pSettings->m_nodeIdx, StringID( GetName() ) );
        return pSettings->m_nodeIdx;
    }

    void ExternalGraphEditorNode::SetName( String const& newName )
    {
        EE_ASSERT( IsRenamable() );
        m_name = GetUniqueSlotName( newName );
    }

    String ExternalGraphEditorNode::GetUniqueSlotName( String const& desiredName )
    {
        // Ensure that the slot name is unique within the same graph
        int32_t cnt = 0;
        String uniqueName = desiredName;
        auto const externalSlotNodes = GetRootGraph()->FindAllNodesOfType<ExternalGraphEditorNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
        for ( int32_t i = 0; i < (int32_t) externalSlotNodes.size(); i++ )
        {
            if ( externalSlotNodes[i] == this )
            {
                continue;
            }

            if ( externalSlotNodes[i]->GetName() == uniqueName )
            {
                uniqueName = String( String::CtorSprintf(), "%s_%d", desiredName.c_str(), cnt );
                cnt++;
                i = 0;
            }
        }

        return uniqueName;
    }

    void ExternalGraphEditorNode::PostPaste()
    {
        EditorGraphNode::PostPaste();
        m_name = GetUniqueSlotName( m_name );
    }

    #if EE_DEVELOPMENT_TOOLS
    void ExternalGraphEditorNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        EditorGraphNode::PostPropertyEdit( pPropertyEdited );
        m_name = GetUniqueSlotName( m_name );
    }
    #endif

}