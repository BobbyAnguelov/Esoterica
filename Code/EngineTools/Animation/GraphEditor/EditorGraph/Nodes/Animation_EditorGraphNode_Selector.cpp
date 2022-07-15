#include "Animation_EditorGraphNode_Selector.h"
#include "Animation_EditorGraphNode_AnimationClip.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Selector.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void SelectorConditionEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );

        CreateInputPin( "Option 0", GraphValueType::Bool );
        CreateInputPin( "Option 1", GraphValueType::Bool );
    }

    //-------------------------------------------------------------------------

    void SelectorEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Option 0", GraphValueType::Pose );
        CreateInputPin( "Option 1", GraphValueType::Pose );

        auto pConditionGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        pConditionGraph->CreateNode<SelectorConditionEditorNode>();

        SetSecondaryGraph( pConditionGraph );
    }

    int16_t SelectorEditorNode::Compile( GraphCompilationContext& context ) const
    {
        SelectorNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<SelectorNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();

            auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionEditorNode>();
            EE_ASSERT( conditionNodes.size() == 1 );
            auto pConditionsNode = conditionNodes[0];
            EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions );

            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<EditorGraphNode>( i );
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

                auto pConditionNode = pConditionsNode->GetConnectedInputNode<EditorGraphNode>( i );
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

    TInlineString<100> SelectorEditorNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        return pinName;
    }

    void SelectorEditorNode::OnDynamicPinCreation( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionEditorNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionsNode = conditionNodes[0];

        int32_t const numOptions = GetNumInputPins();
        EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions - 1 );

        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        pConditionsNode->CreateInputPin( pinName.c_str(), GraphValueType::Bool );
    }

    void SelectorEditorNode::OnDynamicPinDestruction( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionEditorNode>();
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

        pConditionsNode->DestroyInputPin( pintoBeRemovedIdx );
    }

    //-------------------------------------------------------------------------

    void AnimationClipSelectorEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Option 0", GraphValueType::Pose );
        CreateInputPin( "Option 1", GraphValueType::Pose );

        auto pConditionGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        pConditionGraph->CreateNode<SelectorConditionEditorNode>();

        SetSecondaryGraph( pConditionGraph );
    }

    int16_t AnimationClipSelectorEditorNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationClipSelectorNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<AnimationClipSelectorNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();

            auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionEditorNode>();
            EE_ASSERT( conditionNodes.size() == 1 );
            auto pConditionsNode = conditionNodes[0];
            EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions );

            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<EditorGraphNode>( i );
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

                auto pConditionNode = pConditionsNode->GetConnectedInputNode<EditorGraphNode>( i );
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

    TInlineString<100> AnimationClipSelectorEditorNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        return pinName;
    }

    void AnimationClipSelectorEditorNode::OnDynamicPinCreation( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionEditorNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionsNode = conditionNodes[0];

        int32_t const numOptions = GetNumInputPins();
        EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions - 1 );

        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions - 1 );
        pConditionsNode->CreateInputPin( pinName.c_str(), GraphValueType::Bool );
    }

    void AnimationClipSelectorEditorNode::OnDynamicPinDestruction( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionEditorNode>();
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

        pConditionsNode->DestroyInputPin( pintoBeRemovedIdx );
    }

    bool AnimationClipSelectorEditorNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx >= 0 )
        {
            return IsOfType<AnimationClipEditorNode>( pOutputPinNode ) || IsOfType<AnimationClipReferenceEditorNode>( pOutputPinNode );
        }

        return EditorGraphNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }
}