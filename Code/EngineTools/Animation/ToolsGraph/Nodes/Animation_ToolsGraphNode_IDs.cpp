#include "Animation_ToolsGraphNode_IDs.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    IDComparisonToolsNode::IDComparisonToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "ID", GraphValueType::ID );

        m_IDs.emplace_back();
    }

    int16_t IDComparisonToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDComparisonNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDComparisonNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
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

            pDefinition->m_comparison = m_comparison;
            pDefinition->m_comparisionIDs.clear();
            pDefinition->m_comparisionIDs.insert( pDefinition->m_comparisionIDs.begin(), m_IDs.begin(), m_IDs.end() );
        }
        return pDefinition->m_nodeIdx;
    }

    void IDComparisonToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        if ( m_comparison == IDComparisonNode::Comparison::Matches )
        {
            ImGui::Text( "One of these:" );
        }
        else
        {
            ImGui::Text( "NOT any of these:" );
        }

        for ( auto i = 0; i < m_IDs.size(); i++ )
        {
            if ( m_IDs[i].IsValid() )
            {
                ImGui::BulletText( m_IDs[i].c_str() );
            }
            else
            {
                ImGui::BulletText( "INVALID ID" );
            }
        }
    }

    void IDComparisonToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        for ( auto ID : m_IDs )
        {
            outIDs.emplace_back( ID );
        }
    }

    void IDComparisonToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = false;
        for ( auto const ID : m_IDs )
        {
            if ( ID == oldID )
            {
                foundMatch = true;
                break;
            }
        }

        if ( foundMatch )
        {
            NodeGraph::ScopedNodeModification snm( this );
            for ( auto& ID : m_IDs )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    IDToFloatToolsNode::IDToFloatToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "ID", GraphValueType::ID );
    }

    int16_t IDToFloatToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDToFloatNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDToFloatNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( m_mappings.empty() )
            {
                context.LogError( this, "No Mappings Specified For ID To Float Node" );
                return InvalidIndex;
            }

            if ( !ValidateMappings() )
            {
                context.LogError( this, "Invalid Mappings Specified For ID To Float Node" );
                return InvalidIndex;
            }

            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
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

            pDefinition->m_defaultValue = m_defaultValue;

            for ( auto const& mapping : m_mappings )
            {
                pDefinition->m_IDs.emplace_back( mapping.m_ID );
                pDefinition->m_values.emplace_back( mapping.m_value );
            }

            EE_ASSERT( pDefinition->m_IDs.size() == pDefinition->m_values.size() );
        }
        return pDefinition->m_nodeIdx;
    }

    void IDToFloatToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx )
    {
        if ( !m_mappings.empty() && ValidateMappings() )
        {
            for ( auto const& mapping : m_mappings )
            {
                ImGui::Text( "%s = %.3f", mapping.m_ID.c_str(), mapping.m_value );
            }
        }
        else
        {
            ImGui::TextColored( Colors::Red.ToFloat4(), "Invalid Mappings" );
        }
    }

    bool IDToFloatToolsNode::ValidateMappings() const
    {
        for ( auto const& mapping : m_mappings )
        {
            if ( !mapping.m_ID.IsValid() )
            {
                return false;
            }
        }

        return true;
    }

    void IDToFloatToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        for ( auto const& mapping : m_mappings )
        {
            outIDs.emplace_back( mapping.m_ID );
        }
    }

    void IDToFloatToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = false;
        for ( auto const& mapping : m_mappings )
        {
            if ( mapping.m_ID == oldID )
            {
                foundMatch = true;
                break;
            }
        }

        if ( foundMatch )
        {
            NodeGraph::ScopedNodeModification snm( this );
            for ( auto& mapping : m_mappings )
            {
                if ( mapping.m_ID == oldID )
                {
                    mapping.m_ID = newID;
                }
            }
        }
    }
}