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

    bool SelectorConditionToolsNode::DrawPinControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin )
    {
        FlowToolsNode::DrawPinControls( ctx, pUserContext, pin );

        if ( auto pParentSelectorNode = TryCast<SelectorBaseToolsNode>( GetParentNode() ) )
        {
            if ( pin.IsInputPin() && pin.m_type == GetPinTypeForValueType( GraphValueType::Bool ) )
            {
                int32_t const labelIdx = GetInputPinIndex( pin.m_ID );
                String const& pinLabel = pParentSelectorNode->GetPinLabel( labelIdx );
                if ( !pinLabel.empty() )
                {
                    ImGui::Text( "( %s )", pinLabel.c_str());
                    return true;
                }
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    SelectorBaseToolsNode::SelectorBaseToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Option 0", GraphValueType::Pose );
        CreateInputPin( "Option 1", GraphValueType::Pose );

        //-------------------------------------------------------------------------

        m_optionLabels.resize( 2 );
    }

    void SelectorBaseToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        auto pConditionGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        pConditionGraph->CreateNode<SelectorConditionToolsNode>();

        SetSecondaryGraph( pConditionGraph );
    }

    bool SelectorBaseToolsNode::DrawPinControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin )
    {
        FlowToolsNode::DrawPinControls( ctx, pUserContext, pin );

        char buff[255];

        // Add parameter value input field
        if ( pin.IsInputPin() && pin.m_type == GetPinTypeForValueType( GraphValueType::Pose ) )
        {
            int32_t const labelIdx = GetInputPinIndex( pin.m_ID );
            EE_ASSERT( labelIdx >= 0 && labelIdx < m_optionLabels.size() );

            Printf( buff, 255, "%s", m_optionLabels[labelIdx].c_str() );

            ImGui::PushID( &m_optionLabels[labelIdx] );
            ImGui::SetNextItemWidth( 50 * ctx.m_viewScaleFactor );
            bool const wasEdited = ImGui::InputText( "##label", buff, 200, ImGuiInputTextFlags_EnterReturnsTrue );
            if ( wasEdited || ImGui::IsItemDeactivatedAfterEdit() )
            {
                VisualGraph::ScopedNodeModification snm( this );
                m_optionLabels[labelIdx] = buff;
            }
            ImGui::PopID();

            return true;
        }

        return false;
    }

    TInlineString<100> SelectorBaseToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions );
        return pinName;
    }

    void SelectorBaseToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionsNode = conditionNodes[0];

        int32_t const numOptions = GetNumInputPins();
        EE_ASSERT( pConditionsNode->GetNumInputPins() == ( numOptions - 1 ) );

        TInlineString<100> pinName;
        pinName.sprintf( "Option %d", numOptions );
        pConditionsNode->CreateDynamicInputPin( pinName.c_str(), GetPinTypeForValueType( GraphValueType::Bool ) );

        m_optionLabels.emplace_back();
    }

    void SelectorBaseToolsNode::OnDynamicPinDestruction( UUID pinID )
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

    void SelectorBaseToolsNode::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue )
    {
        FlowToolsNode::SerializeCustom( typeRegistry, nodeObjectValue );
        m_optionLabels.resize( GetNumInputPins() );
    }

    //-------------------------------------------------------------------------

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

    //-------------------------------------------------------------------------

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

    bool AnimationClipSelectorToolsNode::IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx >= 0 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
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
        : FlowToolsNode()
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
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }
}