#include "Animation_ToolsGraphNode_IDs.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void IDComparisonToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "ID", GraphValueType::ID );
    }

    int16_t IDComparisonToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDComparisonNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<IDComparisonNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pSettings->m_comparison = m_comparison;
            pSettings->m_comparisionIDs.clear();
            pSettings->m_comparisionIDs.insert( pSettings->m_comparisionIDs.begin(), m_IDs.begin(), m_IDs.end() );
        }
        return pSettings->m_nodeIdx;
    }

    void IDComparisonToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        InlineString infoText;

        if ( m_comparison == IDComparisonNode::Comparison::Matches )
        {
            infoText = "is: ";
        }
        else
        {
            infoText = "is not: ";
        }

        for ( auto i = 0; i < m_IDs.size(); i++ )
        {
            if ( m_IDs[i].IsValid() )
            {
                infoText.append( m_IDs[i].c_str() );
            }
            else
            {
                infoText.append( "INVALID ID" );
            }

            if ( i != m_IDs.size() - 1 )
            {
                infoText.append( ", " );
            }
        }

        ImGui::Text( infoText.c_str() );
    }

    //-------------------------------------------------------------------------

    void IDToFloatToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "ID", GraphValueType::ID );
    }

    int16_t IDToFloatToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDToFloatNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<IDToFloatNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pSettings->m_defaultValue = m_defaultValue;
            
            for ( auto const& mapping : m_mappings )
            {
                pSettings->m_IDs.emplace_back( mapping.m_ID );
                pSettings->m_values.emplace_back( mapping.m_value );
            }

            EE_ASSERT( pSettings->m_IDs.size() == pSettings->m_values.size() );
        }
        return pSettings->m_nodeIdx;
    }
}