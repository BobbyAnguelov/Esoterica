#include "Animation_ToolsGraphNode_BoneMasks.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_BoneMasks.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    BoneMaskToolsNode::BoneMaskToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Bone Mask", GraphValueType::BoneMask, true );
    }

    int16_t BoneMaskToolsNode::Compile( GraphCompilationContext& context ) const
    {
        BoneMaskNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<BoneMaskNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( !m_maskID.IsValid() )
            {
                context.LogError( this, "Invalid Bone Mask ID" );
                return InvalidIndex;
            }

            pSettings->m_boneMaskID = m_maskID;
        }

        return pSettings->m_nodeIdx;
    }

    void BoneMaskToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        DrawInternalSeparator( ctx );

        ImGui::Text( "Mask ID: %s", m_maskID.IsValid() ? m_maskID.c_str() : "NONE");
    }

    void BoneMaskToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        Variation const* pSelectedVariation = pGraphNodeContext->m_pVariationHierarchy->GetVariation( pGraphNodeContext->m_selectedVariationID );

        if ( pSelectedVariation->m_skeleton.GetResourceID().IsValid() )
        {
            pUserContext->RequestOpenResource( pSelectedVariation->m_skeleton.GetResourceID() );
        }
    }

    //-------------------------------------------------------------------------

    BoneMaskBlendToolsNode::BoneMaskBlendToolsNode()
        : FlowToolsNode()
    {
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

    BoneMaskSelectorToolsNode::BoneMaskSelectorToolsNode()
        : FlowToolsNode()
    {
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
            int32_t const numDynamicOptions = numInputs - 2;

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

            // Set parameter values
            //-------------------------------------------------------------------------

            if ( m_parameterValues.size() > numDynamicOptions )
            {
                context.LogWarning( "More parameters set than we have options, extra parameters will be ignored!" );
            }
            pSettings->m_parameterValues.clear();
            pSettings->m_parameterValues.insert( pSettings->m_parameterValues.begin(), m_parameterValues.begin(), m_parameterValues.begin() + numDynamicOptions );

            //-------------------------------------------------------------------------

            pSettings->m_switchDynamically = m_switchDynamically;
            pSettings->m_blendTime = m_blendTime;
        }

        return pSettings->m_nodeIdx;
    }

    TInlineString<100> BoneMaskSelectorToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numOptions = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Mask %d", numOptions - 3 );
        return pinName;
    }
}