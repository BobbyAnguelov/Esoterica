#include "Animation_ToolsGraphNode_Layers.h"
#include "Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    LocalLayerToolsNode::LocalLayerToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Layer", GraphValueType::Special );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Weight", GraphValueType::Float );
        CreateInputPin( "Root Motion Weight", GraphValueType::Float );
        CreateInputPin( "BoneMask", GraphValueType::BoneMask );
    }

    bool LocalLayerToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        if ( IsOfType<StateMachineToolsNode>( pOutputPinNode ) )
        {
            return false;
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    void LocalLayerToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        DrawInternalSeparator( ctx );

        //-------------------------------------------------------------------------

        switch ( m_blendMode )
        {
            case PoseBlendMode::Overlay:
            {
                ImGui::Text( "Overlay" );
            }
            break;

            case PoseBlendMode::Additive:
            {
                ImGui::Text( "Additive" );
            }
            break;

            case PoseBlendMode::GlobalSpace:
            {
                ImGui::Text( "Global Space" );
            }
            break;
        }

        //-------------------------------------------------------------------------

        if ( m_isSynchronized )
        {
            ImGui::Text( "Synchronized" );
        }

        if ( m_ignoreEvents )
        {
            ImGui::Text( "Events Ignored" );
        }
    }

    //-------------------------------------------------------------------------

    StateMachineLayerToolsNode::StateMachineLayerToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Layer", GraphValueType::Special );
        CreateInputPin( "State Machine", GraphValueType::Pose );
    }

    bool StateMachineLayerToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        return IsOfType<StateMachineToolsNode>( pOutputPinNode );
    }

    void StateMachineLayerToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        DrawInternalSeparator( ctx );

        //-------------------------------------------------------------------------

        switch ( m_blendMode )
        {
            case PoseBlendMode::Overlay:
            {
                ImGui::Text( "Overlay" );
            }
            break;

            case PoseBlendMode::Additive:
            {
                ImGui::Text( "Additive" );
            }
            break;

            case PoseBlendMode::GlobalSpace:
            {
                ImGui::Text( "Global Space" );
            }
            break;
        }

        //-------------------------------------------------------------------------

        if ( m_isSynchronized )
        {
            ImGui::Text( "Synchronized" );
        }

        if ( m_ignoreEvents )
        {
            ImGui::Text( "Events Ignored" );
        }
    }

    void StateMachineLayerToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        DrawInfoText( ctx );
        DrawInternalSeparator( ctx );
        BeginDrawInternalRegion( ctx );

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            ImGui::Text( "%.2f", m_runtimeDebugLayerWeight );
        }
        else
        {
            ImGui::Text( "-" );
        }

        EndDrawInternalRegion( ctx );
    }

    //-------------------------------------------------------------------------

    LayerBlendToolsNode::LayerBlendToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Base Node", GraphValueType::Pose );
        CreateInputPin( "Layer 0", GraphValueType::Special );
    }

    bool LayerBlendToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx > 0 )
        {
            return IsOfType<LocalLayerToolsNode>( pOutputPinNode ) || IsOfType<StateMachineLayerToolsNode>( pOutputPinNode );
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    TInlineString<100> LayerBlendToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Layer %d", numOptions - 2 );
        return pinName;
    }

    void LayerBlendToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        int32_t const numOptions = GetNumInputPins();

        int32_t const pintoBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pintoBeRemovedIdx != InvalidIndex );

        // Rename all pins
        //-------------------------------------------------------------------------

        int32_t newPinIdx = 1;
        for ( auto i = 2; i < numOptions; i++ )
        {
            if ( i == pintoBeRemovedIdx )
            {
                continue;
            }

            TInlineString<100> newPinName;
            newPinName.sprintf( "Layer %d", newPinIdx );

            GetInputPin( i )->m_name = newPinName;
            newPinIdx++;
        }
    }

    int16_t LayerBlendToolsNode::Compile( GraphCompilationContext& context ) const
    {
        LayerBlendNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<LayerBlendNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Compile Base
            //-------------------------------------------------------------------------

            auto pBaseNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pBaseNode != nullptr )
            {
                auto compiledNodeIdx = pBaseNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_baseNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected base node pin on layer node!" );
                return InvalidIndex;
            }

            pDefinition->m_onlySampleBaseRootMotion = m_onlySampleBaseRootMotion;

            // Compile Layers
            //-------------------------------------------------------------------------

            bool atLeastOneLayerCompiled = false;

            for ( auto i = 1; i < GetNumInputPins(); i++ )
            {
                if ( auto pLocalLayerNode = GetConnectedInputNode<LocalLayerToolsNode>( i ) )
                {
                    auto& layerDefinition = pDefinition->m_layerDefinition.emplace_back();

                    layerDefinition.m_isSynchronized = pLocalLayerNode->m_isSynchronized;
                    layerDefinition.m_ignoreEvents = pLocalLayerNode->m_ignoreEvents;
                    layerDefinition.m_blendMode = pLocalLayerNode->m_blendMode;
                    layerDefinition.m_isStateMachineLayer = false;

                    // Compile layer node
                    auto pInputNode = pLocalLayerNode->GetConnectedInputNode<FlowToolsNode>( 0 );
                    if ( pInputNode != nullptr )
                    {
                        int16_t const inputNodeIdx = pInputNode->Compile( context );
                        if ( inputNodeIdx != InvalidIndex )
                        {
                            layerDefinition.m_inputNodeIdx = inputNodeIdx;
                            atLeastOneLayerCompiled = true;
                        }
                        else
                        {
                            return InvalidIndex;
                        }
                    }
                    else
                    {
                        InlineString errorMsg( InlineString::CtorSprintf(), "Disconnected layer node pin (%d) on layer node!", i - 1 );
                        context.LogError( this, errorMsg.c_str() );
                        return InvalidIndex;
                    }

                    // Compile optional weight node
                    if ( auto pWeightValueNode = pLocalLayerNode->GetConnectedInputNode<FlowToolsNode>( 1 ) )
                    {
                        auto compiledWeightNodeIdx = pWeightValueNode->Compile( context );
                        if ( compiledWeightNodeIdx != InvalidIndex )
                        {
                            layerDefinition.m_weightValueNodeIdx = compiledWeightNodeIdx;
                        }
                        else
                        {
                            return InvalidIndex;
                        }
                    }

                    // Compile optional root motion weight node
                    if ( auto pRootMotionWeightValueNode = pLocalLayerNode->GetConnectedInputNode<FlowToolsNode>( 2 ) )
                    {
                        auto compiledRootMotionWeightNodeIdx = pRootMotionWeightValueNode->Compile( context );
                        if ( compiledRootMotionWeightNodeIdx != InvalidIndex )
                        {
                            layerDefinition.m_rootMotionWeightValueNodeIdx = compiledRootMotionWeightNodeIdx;
                        }
                        else
                        {
                            return InvalidIndex;
                        }
                    }

                    // Compile optional mask node
                    if ( auto pMaskValueNode = pLocalLayerNode->GetConnectedInputNode<FlowToolsNode>( 3 ) )
                    {
                        auto compiledBoneMaskValueNodeIdx = pMaskValueNode->Compile( context );
                        if ( compiledBoneMaskValueNodeIdx != InvalidIndex )
                        {
                            layerDefinition.m_boneMaskValueNodeIdx = compiledBoneMaskValueNodeIdx;
                        }
                        else
                        {
                            return InvalidIndex;
                        }
                    }
                    else // No Mask
                    {
                        if ( layerDefinition.m_blendMode == PoseBlendMode::GlobalSpace )
                        {
                            context.LogError( this, "Bone Masks are not optional for global blends!" );
                            return InvalidIndex;
                        }
                    }
                }

                // State Machine Layer
                //-------------------------------------------------------------------------

                else if ( auto pStateMachineLayerNode = GetConnectedInputNode<StateMachineLayerToolsNode>( i ) )
                {
                    auto& layerDefinition = pDefinition->m_layerDefinition.emplace_back();

                    layerDefinition.m_isSynchronized = pStateMachineLayerNode->m_isSynchronized;
                    layerDefinition.m_ignoreEvents = pStateMachineLayerNode->m_ignoreEvents;
                    layerDefinition.m_blendMode = pStateMachineLayerNode->m_blendMode;
                    layerDefinition.m_isStateMachineLayer = true;

                    // Compile layer state machine
                    auto pStateMachineNode = pStateMachineLayerNode->GetConnectedInputNode<StateMachineToolsNode>( 0 );
                    if ( pStateMachineNode != nullptr )
                    {
                        auto compiledStateMachineNodeIdx = pStateMachineNode->Compile( context );
                        if ( compiledStateMachineNodeIdx != InvalidIndex )
                        {
                            layerDefinition.m_inputNodeIdx = compiledStateMachineNodeIdx;
                            atLeastOneLayerCompiled = true;
                        }
                        else
                        {
                            return InvalidIndex;
                        }
                    }
                    else
                    {
                        context.LogError( this, "Disconnected base node pin on layer node!" );
                        return InvalidIndex;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( !atLeastOneLayerCompiled )
            {
                context.LogError( this, "No connected layers on layer node!" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    void LayerBlendToolsNode::PreDrawUpdate( VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        bool const isPreviewing = pGraphNodeContext->HasDebugData();
        int16_t const runtimeNodeIdx = isPreviewing ? pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() ) : InvalidIndex;
        bool const isPreviewingAndValidRuntimeNodeIdx = isPreviewing && ( runtimeNodeIdx != InvalidIndex );

        if ( isPreviewingAndValidRuntimeNodeIdx )
        {
            for ( auto i = 1; i < GetNumInputPins(); i++ )
            {
                if ( auto pStateMachineLayerNode = GetConnectedInputNode<StateMachineLayerToolsNode>( i ) )
                {
                    auto pLayerNode = static_cast<GraphNodes::LayerBlendNode const*>( pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx ) );
                    if ( pLayerNode->IsInitialized() )
                    {
                        pStateMachineLayerNode->m_runtimeDebugLayerWeight = pLayerNode->GetLayerWeight( i - 1 );
                    }
                    else
                    {
                        pStateMachineLayerNode->m_runtimeDebugLayerWeight = 0.0f;
                    }
                }
            }
        }
    }
}