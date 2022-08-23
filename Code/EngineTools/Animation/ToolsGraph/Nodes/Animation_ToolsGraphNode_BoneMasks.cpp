#include "Animation_ToolsGraphNode_BoneMasks.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_BoneMasks.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void BoneMaskToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        DataSlotToolsNode::Initialize( pParent );
        CreateOutputPin( "Bone Mask", GraphValueType::BoneMask, true );
    }

    int16_t BoneMaskToolsNode::Compile( GraphCompilationContext& context ) const
    {
        BoneMaskNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<BoneMaskNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_rootMotionWeight = m_rootMotionWeight;
            pSettings->m_dataSlotIdx = context.RegisterDataSlotNode( GetID() );
        }

        return pSettings->m_nodeIdx;
    }

    ResourceTypeID BoneMaskToolsNode::GetSlotResourceTypeID() const
    {
        return BoneMaskDefinition::GetStaticResourceTypeID();
    }

    //-------------------------------------------------------------------------

    void BoneMaskBlendToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::BoneMask, true );
        CreateInputPin( "Blend Weight", GraphValueType::Float );
        CreateInputPin( "Source", GraphValueType::BoneMask );
        CreateInputPin( "Target", GraphValueType::BoneMask );
    }

    int16_t BoneMaskBlendToolsNode::Compile( GraphCompilationContext& context ) const
    {
        BoneMaskBlendNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<BoneMaskBlendNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_blendWeightValueNodeIdx = compiledNodeIdx;
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

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_sourceMaskNodeIdx = compiledNodeIdx;
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

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_targetMaskNodeIdx = compiledNodeIdx;
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
        }

        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    void BoneMaskSelectorToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        CreateOutputPin( "Result", GraphValueType::BoneMask, true );
        CreateInputPin( "Parameter", GraphValueType::ID );
        CreateInputPin( "Default Mask", GraphValueType::BoneMask );
        CreateInputPin( "Mask 0", GraphValueType::BoneMask );
    }

    int16_t BoneMaskSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        BoneMaskSelectorNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<BoneMaskSelectorNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                int16_t const compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_parameterValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on selector node!" );
                return InvalidIndex;
            }

            // Default Mask
            //-------------------------------------------------------------------------

            auto pDefaultMaskNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pDefaultMaskNode != nullptr )
            {
                int16_t const compiledNodeIdx = pDefaultMaskNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_defaultMaskNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected default  input pin on selector node!" );
                return InvalidIndex;
            }

            // Dynamic Options
            //-------------------------------------------------------------------------

            int32_t const numInputs = GetNumInputPins();

            for ( auto i = 2; i < numInputs; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_maskNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected option pin on selector node!" );
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pSettings->m_switchDynamically = m_switchDynamically;
            pSettings->m_blendTime = m_blendTime;

            pSettings->m_parameterValues.clear();
            pSettings->m_parameterValues.insert( pSettings->m_parameterValues.begin(), m_parameterValues.begin(), m_parameterValues.end() );
        }

        return pSettings->m_nodeIdx;
    }
}