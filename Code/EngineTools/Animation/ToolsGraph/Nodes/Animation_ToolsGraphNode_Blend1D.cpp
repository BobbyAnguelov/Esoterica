#include "Animation_ToolsGraphNode_Blend1D.h"
#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    Blend1DToolsNode::Blend1DToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );

        m_blendSpace.back().m_value = 1.0f;
    }

    int16_t Blend1DToolsNode::Compile( GraphCompilationContext & context ) const
    {
        Blend1DNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<Blend1DNode>( this, pDefinition );
        if ( state != NodeCompilationState::NeedCompilation )
        {
            return pDefinition->m_nodeIdx;
        }

        if ( m_blendSpace.size() <= 1 )
        {
            context.LogError( this, "Not enough points to generate a blendspace!" );
            return InvalidIndex;
        }

        // Validate and create parameterization
        //-------------------------------------------------------------------------

        if ( m_blendSpace.size() != ( GetNumInputPins() - 1 ) )
        {
            context.LogError( this, "The number of blend space points dont match number of input points. Node is in an invalid state!" );
            return InvalidIndex;
        }

        TInlineVector<float, 5> values;
        for ( auto const& point : m_blendSpace )
        {
            if ( VectorContains( values, point.m_value ) )
            {
                context.LogError( this, "Duplicate blend space value detected. All blend space points are required to have unique values!" );
                return InvalidIndex;
            }

            values.emplace_back( point.m_value );
        }

        // Sort the values so they match the pin order
        eastl::sort( values.begin(), values.end() );
        pDefinition->m_parameterization = ParameterizedBlendNode::Parameterization::CreateParameterization( values );

        // Parameter
        //-------------------------------------------------------------------------

        auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
        if ( pParameterNode != nullptr )
        {
            int16_t const compiledNodeIdx = pParameterNode->Compile( context );
            if ( compiledNodeIdx != InvalidIndex )
            {
                pDefinition->m_inputParameterValueNodeIdx = compiledNodeIdx;
            }
            else
            {
                return InvalidIndex;
            }
        }
        else
        {
            context.LogError( this, "Disconnected parameter pin on blend node!" );
            return InvalidIndex;
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
                    pDefinition->m_sourceNodeIndices.emplace_back( compiledNodeIdx );
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected source pin on blend node!" );
                return InvalidIndex;
            }
        }

        //-------------------------------------------------------------------------

        pDefinition->m_allowLooping = m_allowLooping;

        return pDefinition->m_nodeIdx;
    }

    void Blend1DToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_blendSpace" ) )
        {
            eastl::sort( m_blendSpace.begin(), m_blendSpace.end() );
            RefreshDynamicPins();
        }
    }

    void Blend1DToolsNode::RefreshDynamicPins()
    {
        // Reorder the input pins
        //-------------------------------------------------------------------------

        TVector<UUID> newOrder;
        for ( int32_t i = 0; i < m_blendSpace.size(); i++ )
        {
            newOrder.emplace_back( m_blendSpace[i].m_pinID );
        }

        ReorderInputPins( newOrder );

        // Rename pins
        //-------------------------------------------------------------------------

        for ( int32_t i = 1; i < GetNumInputPins(); i++ )
        {
            NodeGraph::Pin* pInputPin = GetInputPin( i );
            BlendSpacePoint const& point = m_blendSpace[i - 1];
            pInputPin->m_name.sprintf( "%s (%.2f)", point.m_name.empty() ? "Input" : point.m_name.c_str(), point.m_value );
        }
    }

    void Blend1DToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        m_blendSpace.emplace_back( GetInputPin( pinID )->m_name, 0.0f, pinID );
        eastl::sort( m_blendSpace.begin(), m_blendSpace.end() );
        RefreshDynamicPins();
    }

    void Blend1DToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        int32_t const pointIdx = pinToBeRemovedIdx - 1;
        EE_ASSERT( pointIdx >= 0 && pointIdx < m_blendSpace.size() );
        m_blendSpace.erase( m_blendSpace.begin() + pointIdx );
    }

    void Blend1DToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        FlowToolsNode::DrawExtraControls( ctx, pUserContext );

        //-------------------------------------------------------------------------

        int16_t sourceRuntimeIdx0 = InvalidIndex;
        int16_t sourceRuntimeIdx1 = InvalidIndex;
        float blendWeight = 0.0f;

        // Get node debug info
        //-------------------------------------------------------------------------

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        int16_t const runtimeNodeIdx = pGraphNodeContext->HasDebugData() ? pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() ) : InvalidIndex;
        if ( runtimeNodeIdx != InvalidIndex )
        {
            auto pBlendNode = static_cast<GraphNodes::Blend1DNode const*>( pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx ) );
            if ( pBlendNode->WasInitialized() )
            {
                pBlendNode->GetDebugInfo( sourceRuntimeIdx0, sourceRuntimeIdx1, blendWeight );
            }
        }

        // Look up point names
        //-------------------------------------------------------------------------

        auto GetLabelText = [&] ( InlineString& outLabel, int16_t sourceRuntimeIdx )
        {
            for ( int32_t i = 1; i < GetNumInputPins(); i++ )
            {
                int16_t const inputNodeRuntimeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetConnectedInputNode( i )->GetID() );
                int32_t const blendPointIdx = i - 1;

                if ( inputNodeRuntimeIdx == sourceRuntimeIdx )
                {
                    if ( m_blendSpace[blendPointIdx].m_name.empty() )
                    {
                        outLabel.sprintf( "Point %d", blendPointIdx );
                    }
                    else
                    {
                        outLabel = m_blendSpace[blendPointIdx].m_name.c_str();
                    }
                }
            }
        };

        //-------------------------------------------------------------------------

        ImVec2 const visualizationSize( 200, 40 );
        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_colorGray8 );
        if ( ImGui::BeginChild( "Viz", visualizationSize ) )
        {
            if ( runtimeNodeIdx == InvalidIndex )
            {
                // Draw nothing
            }
            else
            {
                auto pDrawList = ImGui::GetWindowDrawList();
                ImVec2 const windowPos = ImGui::GetWindowPos();
                ImVec2 const windowSize = ImGui::GetWindowSize();
                float const windowHalfHeight = windowSize.y / 2;

                bool const isSinglePoint = ( sourceRuntimeIdx1 == InvalidIndex );
                if ( isSinglePoint )
                {
                    InlineString label;
                    GetLabelText( label, sourceRuntimeIdx0 );
                    ImVec2 const textSize = ImGui::CalcTextSize( label.c_str() );
                    ImVec2 const textPos = windowPos + ImVec2( ( windowSize.x - textSize.x ) / 2, ( windowSize.y - textSize.y ) / 2 );
                    pDrawList->AddText( textPos, Colors::Gold, label.c_str() );
                }
                else // Blend
                {
                    pDrawList->AddLine( windowPos + ImVec2( 0, windowHalfHeight ), windowPos + ImVec2( windowSize.x, windowHalfHeight ), Colors::GreenYellow );

                    InlineString label0;
                    GetLabelText( label0, sourceRuntimeIdx0 );
                    pDrawList->AddText( windowPos, Colors::Gold, label0.c_str() );

                    InlineString label1;
                    GetLabelText( label1, sourceRuntimeIdx1 );
                    ImVec2 const textSize = ImGui::CalcTextSize( label1.c_str() );
                    pDrawList->AddText( windowPos + windowSize - textSize, Colors::Gold, label1.c_str() );

                    ImVec2 blendPointPos( windowPos.x + ( blendWeight * windowSize.x ), windowPos.y + windowHalfHeight );
                    pDrawList->AddCircleFilled( blendPointPos, 4.0f, Colors::Gold );
                }
            }
            
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    //-------------------------------------------------------------------------

    VelocityBlendToolsNode::VelocityBlendToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Input", GraphValueType::Pose );
    }

    TInlineString<100> VelocityBlendToolsNode::GetNewDynamicInputPinName() const
    {
        return TInlineString<100>( TInlineString<100>::CtorSprintf(), "Input %d", GetNumInputPins() - 2 );
    }

    bool VelocityBlendToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx > 0 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    int16_t VelocityBlendToolsNode::Compile( GraphCompilationContext& context ) const
    {
        VelocityBlendNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<VelocityBlendNode>( this, pDefinition );

        if ( state != NodeCompilationState::NeedCompilation )
        {
            return pDefinition->m_nodeIdx;
        }

        // Parameter
        //-------------------------------------------------------------------------

        auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
        if ( pParameterNode != nullptr )
        {
            int16_t const compiledNodeIdx = pParameterNode->Compile( context );
            if ( compiledNodeIdx != InvalidIndex )
            {
                pDefinition->m_inputParameterValueNodeIdx = compiledNodeIdx;
            }
            else
            {
                return InvalidIndex;
            }
        }
        else
        {
            context.LogError( this, "Disconnected parameter pin on blend node!" );
            return InvalidIndex;
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
                    pDefinition->m_sourceNodeIndices.emplace_back( compiledNodeIdx );
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected source pin on blend node!" );
                return InvalidIndex;
            }
        }

        //-------------------------------------------------------------------------

        pDefinition->m_allowLooping = m_allowLooping;

        return pDefinition->m_nodeIdx;
    }
}