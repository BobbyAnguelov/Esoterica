#include "Animation_ToolsGraphNode_Blends.h"
#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ParameterizedBlendToolsNode::ParameterizedBlendToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
    }

    bool ParameterizedBlendToolsNode::CompileParameterAndSourceNodes( GraphCompilationContext& context, ParameterizedBlendNode::Settings* pSettings ) const
    {
        // Parameter
        //-------------------------------------------------------------------------

        auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
        if ( pParameterNode != nullptr )
        {
            int16_t const compiledNodeIdx = pParameterNode->Compile( context );
            if ( compiledNodeIdx != InvalidIndex )
            {
                pSettings->m_inputParameterValueNodeIdx = compiledNodeIdx;
            }
            else
            {
                return false;
            }
        }
        else
        {
            context.LogError( this, "Disconnected parameter pin on blend node!" );
            return false;
        }

        // Sources
        //-------------------------------------------------------------------------

        for ( auto i = 1; i < GetNumInputPins(); i++ )
        {
            auto pSourceNode = GetConnectedInputNode<FlowToolsNode>( i );
            if ( pSourceNode != nullptr )
            {
                int16_t const compiledNodeIdx = pSourceNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_sourceNodeIndices.emplace_back( compiledNodeIdx );
                }
                else
                {
                    return false;
                }
            }
            else
            {
                context.LogError( this, "Disconnected source pin on blend node!" );
                return false;
            }
        }

        // Common Settings
        //-------------------------------------------------------------------------

        pSettings->m_isSynchronized = m_isSynchronized;

        return true;
    }

    //-------------------------------------------------------------------------

    RangedBlendToolsNode::RangedBlendToolsNode()
        : ParameterizedBlendToolsNode()
    {
        m_parameterValues.emplace_back( 0.0f );
        m_parameterValues.emplace_back( 1.0f );
    }

    int16_t RangedBlendToolsNode::Compile( GraphCompilationContext & context ) const
    {
        RangedBlendNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<RangedBlendNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( !CompileParameterAndSourceNodes( context, pSettings ) )
            {
                return false;
                //return InvalidIndex;
            }

            // Create parameterization
            //-------------------------------------------------------------------------

            EE_ASSERT( m_parameterValues.size() == ( GetNumInputPins() - 1 ) );
            TInlineVector<float, 5> values( m_parameterValues.begin(), m_parameterValues.end() );
            pSettings->m_parameterization = ParameterizedBlendNode::Parameterization::CreateParameterization( values );
        }

        return pSettings->m_nodeIdx;
    }

    bool RangedBlendToolsNode::DrawPinControls( VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin )
    {
        ParameterizedBlendToolsNode::DrawPinControls( pUserContext, pin );

        // Add parameter value input field
        if ( pin.IsInputPin() && pin.m_type == (uint32_t) GraphValueType::Pose )
        {
            int32_t const pinIdx = GetInputPinIndex( pin.m_ID );
            int32_t const parameterIdx = pinIdx - 1;
            EE_ASSERT( parameterIdx >= 0 && parameterIdx < m_parameterValues.size() );

            ImGui::PushID( &m_parameterValues[parameterIdx] );
            ImGui::SetNextItemWidth( 50 );
            ImGui::InputFloat( "##parameter", &m_parameterValues[parameterIdx], 0.0f, 0.0f, "%.2f" );
            ImGui::PopID();

            return true;
        }

        return false;
    }

    void RangedBlendToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        m_parameterValues.emplace_back( 0.0f );
    }

    void RangedBlendToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        int32_t const parameterIdx = pinToBeRemovedIdx - 1;
        EE_ASSERT( parameterIdx >= 0 && parameterIdx < m_parameterValues.size() );
        m_parameterValues.erase( m_parameterValues.begin() + parameterIdx );
    }

    //-------------------------------------------------------------------------

    int16_t VelocityBlendToolsNode::Compile( GraphCompilationContext& context ) const
    {
        VelocityBlendNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<VelocityBlendNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( !CompileParameterAndSourceNodes( context, pSettings ) )
            {
                return InvalidIndex;
            }
        }

        return pSettings->m_nodeIdx;
    }

    bool VelocityBlendToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx > 0 )
        {
            return IsOfType<AnimationClipToolsNode>( pOutputPinNode ) || IsOfType<AnimationClipReferenceToolsNode>( pOutputPinNode );
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }
}