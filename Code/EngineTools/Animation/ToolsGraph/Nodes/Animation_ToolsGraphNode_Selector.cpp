#include "Animation_ToolsGraphNode_Selector.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Selector.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    SelectorConditionToolsNode::SelectorConditionToolsNode()
        : FlowToolsNode()
    {
        CreateInputPin( "Option 0", GraphValueType::Bool );
        CreateInputPin( "Option 1", GraphValueType::Bool );
    }

    //-------------------------------------------------------------------------

    SelectorToolsNode::SelectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Option 0", GraphValueType::Pose );
        CreateInputPin( "Option 1", GraphValueType::Pose );
    }

    void SelectorToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        auto pConditionGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        pConditionGraph->CreateNode<SelectorConditionToolsNode>();

        SetSecondaryGraph( pConditionGraph );
    }

    int16_t SelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        SelectorNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<SelectorNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();

            auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
            EE_ASSERT( conditionNodes.size() == 1 );
            auto pConditionsNode = conditionNodes[0];
            EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions );

            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_optionNodeIndices.emplace_back( compiledNodeIdx );
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

                auto pConditionNode = pConditionsNode->GetConnectedInputNode<FlowToolsNode>( i );
                if ( pConditionNode != nullptr )
                {
                    auto compiledNodeIdx = pConditionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected condition pin on selector node!" );
                    return InvalidIndex;
                }
            }

            EE_ASSERT( pSettings->m_optionNodeIndices.size() == pSettings->m_conditionNodeIndices.size() );

            //-------------------------------------------------------------------------

            if ( pSettings->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }
        return pSettings->m_nodeIdx;
    }

    TInlineString<100> SelectorToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        return pinName;
    }

    void SelectorToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionsNode = conditionNodes[0];

        int32_t const numOptions = GetNumInputPins();
        EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions - 1 );

        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        pConditionsNode->CreateDynamicInputPin( pinName.c_str(), (uint32_t) GraphValueType::Bool );
    }

    void SelectorToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionsNode = conditionNodes[0];

        int32_t const numOptions = GetNumInputPins();
        EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions );

        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        // Rename all pins
        //-------------------------------------------------------------------------

        int32_t newPinIdx = 2;
        for ( auto i = 2; i < numOptions; i++ )
        {
            if ( i == pinToBeRemovedIdx )
            {
                continue;
            }

            TInlineString<100> newPinName;
            newPinName.sprintf( "Option %d", newPinIdx );

            GetInputPin( i )->m_name = newPinName;
            pConditionsNode->GetInputPin( i )->m_name = newPinName;
            newPinIdx++;
        }

        // Destroy condition node pin
        //-------------------------------------------------------------------------

        pConditionsNode->DestroyDynamicInputPin( pConditionsNode->GetInputPin( pinToBeRemovedIdx )->m_ID );
    }

    //-------------------------------------------------------------------------

    AnimationClipSelectorToolsNode::AnimationClipSelectorToolsNode()
        : AnimationClipReferenceToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Option 0", GraphValueType::Pose );
        CreateInputPin( "Option 1", GraphValueType::Pose );
    }

    void AnimationClipSelectorToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        auto pConditionGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        pConditionGraph->CreateNode<SelectorConditionToolsNode>();

        SetSecondaryGraph( pConditionGraph );
    }

    int16_t AnimationClipSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationClipSelectorNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<AnimationClipSelectorNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();

            auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
            EE_ASSERT( conditionNodes.size() == 1 );
            auto pConditionsNode = conditionNodes[0];
            EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions );

            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_optionNodeIndices.emplace_back( compiledNodeIdx );
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

                auto pConditionNode = pConditionsNode->GetConnectedInputNode<FlowToolsNode>( i );
                if ( pConditionNode != nullptr )
                {
                    auto compiledNodeIdx = pConditionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected condition pin on selector node!" );
                    return InvalidIndex;
                }
            }

            EE_ASSERT( pSettings->m_optionNodeIndices.size() == pSettings->m_conditionNodeIndices.size() );

            //-------------------------------------------------------------------------

            if ( pSettings->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }
        return pSettings->m_nodeIdx;
    }

    TInlineString<100> AnimationClipSelectorToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        return pinName;
    }

    void AnimationClipSelectorToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionsNode = conditionNodes[0];

        int32_t const numOptions = GetNumInputPins();
        EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions - 1 );

        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        pConditionsNode->CreateDynamicInputPin( pinName.c_str(), (uint32_t) GraphValueType::Bool );
    }

    void AnimationClipSelectorToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionsNode = conditionNodes[0];

        int32_t const numOptions = GetNumInputPins();
        EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions );

        int32_t const pintoBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pintoBeRemovedIdx != InvalidIndex );

        // Rename all pins
        //-------------------------------------------------------------------------

        int32_t newPinIdx = 2;
        for ( auto i = 2; i < numOptions; i++ )
        {
            if ( i == pintoBeRemovedIdx )
            {
                continue;
            }

            TInlineString<100> newPinName;
            newPinName.sprintf( "Option %d", newPinIdx );

            GetInputPin( i )->m_name = newPinName;
            pConditionsNode->GetInputPin( i )->m_name = newPinName;
            newPinIdx++;
        }

        // Destroy condition node pin
        //-------------------------------------------------------------------------

        pConditionsNode->DestroyDynamicInputPin( pConditionsNode->GetInputPin( pintoBeRemovedIdx )->m_ID );
    }

    bool AnimationClipSelectorToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx >= 0 )
        {
            return IsOfType<AnimationClipToolsNode>( pOutputPinNode ) || IsOfType<AnimationClipReferenceToolsNode>( pOutputPinNode );
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    //-------------------------------------------------------------------------

    ParameterizedSelectorToolsNode::ParameterizedSelectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateInputPin( "Option 0", GraphValueType::Pose );
        CreateInputPin( "Option 1", GraphValueType::Pose );
    }

    int16_t ParameterizedSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ParameterizedSelectorNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ParameterizedSelectorNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Compile Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                auto compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_parameterNodeIdx = compiledNodeIdx;
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

            // Compile Options
            //-------------------------------------------------------------------------

            int32_t const numInputPins = GetNumInputPins();
            for ( auto i = 1; i < numInputPins; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_optionNodeIndices.emplace_back( compiledNodeIdx );
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

            if ( pSettings->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }
        return pSettings->m_nodeIdx;
    }

    TInlineString<100> ParameterizedSelectorToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numInputPins = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numInputPins - 2 );
        return pinName;
    }

    void ParameterizedSelectorToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        int32_t const numInputPins = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numInputPins - 1 );
    }

    void ParameterizedSelectorToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        // Rename all pins
        //-------------------------------------------------------------------------

        int32_t newPinIdx = 2;
        int32_t const numInputPins = GetNumInputPins();
        for ( auto i = 3; i < numInputPins; i++ )
        {
            if ( i == pinToBeRemovedIdx )
            {
                continue;
            }

            TInlineString<100> newPinName;
            newPinName.sprintf( "Option %d", newPinIdx );

            GetInputPin( i )->m_name = newPinName;
            newPinIdx++;
        }
    }

    //-------------------------------------------------------------------------

    ParameterizedAnimationClipSelectorToolsNode::ParameterizedAnimationClipSelectorToolsNode()
        : AnimationClipReferenceToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateInputPin( "Option 0", GraphValueType::Pose );
        CreateInputPin( "Option 1", GraphValueType::Pose );
    }

    int16_t ParameterizedAnimationClipSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ParameterizedAnimationClipSelectorNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<ParameterizedAnimationClipSelectorNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Compile Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                auto compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_parameterNodeIdx = compiledNodeIdx;
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

            // Compile Options
            //-------------------------------------------------------------------------

            int32_t const numInputPins = GetNumInputPins();
            for ( auto i = 1; i < numInputPins; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_optionNodeIndices.emplace_back( compiledNodeIdx );
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

            if ( pSettings->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }
        return pSettings->m_nodeIdx;
    }

    TInlineString<100> ParameterizedAnimationClipSelectorToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numInputPins = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numInputPins - 2 );
        return pinName;
    }

    void ParameterizedAnimationClipSelectorToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        int32_t const numInputPins = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numInputPins - 1 );
    }

    void ParameterizedAnimationClipSelectorToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        // Rename all pins
        //-------------------------------------------------------------------------

        int32_t newPinIdx = 2;
        int32_t const numInputPins = GetNumInputPins();
        for ( auto i = 3; i < numInputPins; i++ )
        {
            if ( i == pinToBeRemovedIdx )
            {
                continue;
            }

            TInlineString<100> newPinName;
            newPinName.sprintf( "Option %d", newPinIdx );

            GetInputPin( i )->m_name = newPinName;
            newPinIdx++;
        }
    }

    bool ParameterizedAnimationClipSelectorToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx >= 1 )
        {
            return IsOfType<AnimationClipToolsNode>( pOutputPinNode ) || IsOfType<AnimationClipReferenceToolsNode>( pOutputPinNode );
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }
}