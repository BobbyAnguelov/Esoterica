#include "Animation_ToolsGraphNode_Layers.h"
#include "Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Layers.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void LocalLayerToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );

        CreateOutputPin( "Layer", GraphValueType::Special );
        CreateInputPin( "Input", GraphValueType::Pose );
        CreateInputPin( "Weight", GraphValueType::Float );
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
            case PoseBlendMode::Interpolative:
            {
                ImGui::Text( "Interpolate" );
            }
            break;

            case PoseBlendMode::Additive:
            {
                ImGui::Text( "Additive" );
            }
            break;

            case PoseBlendMode::InterpolativeGlobalSpace:
            {
                ImGui::Text( "Interpolate (Global)" );
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

    void StateMachineLayerToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );

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
            case PoseBlendMode::Interpolative:
            {
                ImGui::Text( "Interpolate" );
            }
            break;

            case PoseBlendMode::Additive:
            {
                ImGui::Text( "Additive" );
            }
            break;

            case PoseBlendMode::InterpolativeGlobalSpace:
            {
                ImGui::Text( "Interpolate (Global)" );
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

    void LayerBlendToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );

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
        LayerBlendNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<LayerBlendNode>( this, pSettings );
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
                    pSettings->m_baseNodeIdx = compiledNodeIdx;
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

            pSettings->m_onlySampleBaseRootMotion = m_onlySampleBaseRootMotion;

            // Compile Layers
            //-------------------------------------------------------------------------

            bool atLeastOneLayerCompiled = false;

            for ( auto i = 1; i < GetNumInputPins(); i++ )
            {
                if ( auto pLocalLayerNode = GetConnectedInputNode<LocalLayerToolsNode>( i ) )
                {
                    auto& layerSettings = pSettings->m_layerSettings.emplace_back();

                    layerSettings.m_isSynchronized = pLocalLayerNode->m_isSynchronized;
                    layerSettings.m_ignoreEvents = pLocalLayerNode->m_ignoreEvents;
                    layerSettings.m_blendMode = pLocalLayerNode->m_blendMode;
                    layerSettings.m_isStateMachineLayer = false;

                    // Compile layer state machine
                    auto pInputNode = pLocalLayerNode->GetConnectedInputNode<FlowToolsNode>( 0 );
                    if ( pInputNode != nullptr )
                    {
                        int16_t const inputNodeIdx = pInputNode->Compile( context );
                        if ( inputNodeIdx != InvalidIndex )
                        {
                            layerSettings.m_inputNodeIdx = inputNodeIdx;
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

                    // Compile optional weight node
                    if ( auto pWeightValueNode = pLocalLayerNode->GetConnectedInputNode<FlowToolsNode>( 1 ) )
                    {
                        auto compiledWeightNodeIdx = pWeightValueNode->Compile( context );
                        if ( compiledWeightNodeIdx != InvalidIndex )
                        {
                            layerSettings.m_weightValueNodeIdx = compiledWeightNodeIdx;
                        }
                        else
                        {
                            return InvalidIndex;
                        }
                    }

                    // Compile optional mask node
                    if ( auto pMaskValueNode = pLocalLayerNode->GetConnectedInputNode<FlowToolsNode>( 2 ) )
                    {
                        auto compiledBoneMaskValueNodeIdx = pMaskValueNode->Compile( context );
                        if ( compiledBoneMaskValueNodeIdx != InvalidIndex )
                        {
                            layerSettings.m_boneMaskValueNodeIdx = compiledBoneMaskValueNodeIdx;
                        }
                        else
                        {
                            return InvalidIndex;
                        }
                    }
                }
                else if ( auto pStateMachineLayerNode = GetConnectedInputNode<StateMachineLayerToolsNode>( i ) )
                {
                    auto& layerSettings = pSettings->m_layerSettings.emplace_back();

                    layerSettings.m_isSynchronized = pStateMachineLayerNode->m_isSynchronized;
                    layerSettings.m_ignoreEvents = pStateMachineLayerNode->m_ignoreEvents;
                    layerSettings.m_blendMode = pStateMachineLayerNode->m_blendMode;
                    layerSettings.m_isStateMachineLayer = true;

                    // Compile layer state machine
                    auto pStateMachineNode = pStateMachineLayerNode->GetConnectedInputNode<StateMachineToolsNode>( 0 );
                    if ( pStateMachineNode != nullptr )
                    {
                        auto compiledStateMachineNodeIdx = pStateMachineNode->Compile( context );
                        if ( compiledStateMachineNodeIdx != InvalidIndex )
                        {
                            layerSettings.m_inputNodeIdx = compiledStateMachineNodeIdx;
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
        return pSettings->m_nodeIdx;
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
                    pStateMachineLayerNode->m_runtimeDebugLayerWeight = pGraphNodeContext->GetLayerWeight( runtimeNodeIdx, i - 1 );
                }
            }
        }
    }
}