#include "Animation_ToolsGraphNode_Layers.h"
#include "Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Layers.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void LayerSettingsToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );

        CreateOutputPin( "Layer", GraphValueType::Unknown );
        CreateInputPin( "State Machine", GraphValueType::Pose );
    }

    bool LayerSettingsToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        return IsOfType<StateMachineToolsNode>( pOutputPinNode );
    }

    //-------------------------------------------------------------------------

    void LayerToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Base Node", GraphValueType::Pose );
        CreateInputPin( "Layer 0", GraphValueType::Unknown );
    }

    bool LayerToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx > 0 )
        {
            return IsOfType<LayerSettingsToolsNode>( pOutputPinNode );
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    TInlineString<100> LayerToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Layer %d", numOptions - 2 );
        return pinName;
    }

    void LayerToolsNode::OnDynamicPinDestruction( UUID pinID )
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

    int16_t LayerToolsNode::Compile( GraphCompilationContext& context ) const
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
                auto pLayerSettingsNode = GetConnectedInputNode<LayerSettingsToolsNode>( i );
                if ( pLayerSettingsNode != nullptr )
                {
                    auto& layerSettings = pSettings->m_layerSettings.emplace_back();

                    layerSettings.m_isSynchronized = pLayerSettingsNode->m_isSynchronized;
                    layerSettings.m_ignoreEvents = pLayerSettingsNode->m_ignoreEvents;
                    layerSettings.m_blendOptions = pLayerSettingsNode->m_blendOptions;

                    // Compile layer state machine
                    auto pLayerStateMachineNode = pLayerSettingsNode->GetConnectedInputNode<StateMachineToolsNode>( 0 );
                    if ( pLayerStateMachineNode != nullptr )
                    {
                        auto compiledStateMachineNodeIdx = pLayerStateMachineNode->Compile( context );
                        if ( compiledStateMachineNodeIdx != InvalidIndex )
                        {
                            layerSettings.m_layerNodeIdx = compiledStateMachineNodeIdx;
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

            if ( !atLeastOneLayerCompiled )
            {
                context.LogError( this, "No connected layers on layer node!" );
                return InvalidIndex;
            }
        }
        return pSettings->m_nodeIdx;
    }
}