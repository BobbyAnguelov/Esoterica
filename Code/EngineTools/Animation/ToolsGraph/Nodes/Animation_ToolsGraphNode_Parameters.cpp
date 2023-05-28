#include "Animation_ToolsGraphNode_Parameters.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Parameters.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ControlParameterToolsNode::ControlParameterToolsNode( String const& name, String const& categoryName )
        : m_name( name )
        , m_parameterCategory( categoryName )
    {
        EE_ASSERT( !m_name.empty() );
    }

    int16_t ControlParameterToolsNode::Compile( GraphCompilationContext& context ) const
    {
        switch ( GetValueType() )
        {
            case GraphValueType::Bool:
            {
                GraphNodes::ControlParameterBoolNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterBoolNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::ID:
            {
                GraphNodes::ControlParameterIDNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterIDNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::Int:
            {
                GraphNodes::ControlParameterIntNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterIntNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::Float:
            {
                GraphNodes::ControlParameterFloatNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterFloatNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::Vector:
            {
                GraphNodes::ControlParameterVectorNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterVectorNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::Target:
            {
                GraphNodes::ControlParameterTargetNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterTargetNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            return InvalidIndex;
            break;
        }
    }

    void ControlParameterToolsNode::Rename( String const& name, String const& category )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_name = name;
        m_parameterCategory = category;
    }

    //-------------------------------------------------------------------------

    BoolControlParameterToolsNode::BoolControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    BoolControlParameterToolsNode::BoolControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    //-------------------------------------------------------------------------

    FloatControlParameterToolsNode::FloatControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    FloatControlParameterToolsNode::FloatControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    //-------------------------------------------------------------------------

    IntControlParameterToolsNode::IntControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Int, true );
    }

    IntControlParameterToolsNode::IntControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Int, true );
    }

    //-------------------------------------------------------------------------

    IDControlParameterToolsNode::IDControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    IDControlParameterToolsNode::IDControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    //-------------------------------------------------------------------------

    VectorControlParameterToolsNode::VectorControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    VectorControlParameterToolsNode::VectorControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    //-------------------------------------------------------------------------

    TargetControlParameterToolsNode::TargetControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    TargetControlParameterToolsNode::TargetControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    void TargetControlParameterToolsNode::SetPreviewTargetTransform( Transform const& transform )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_previewStartTransform = transform;
    }

    Target TargetControlParameterToolsNode::GetPreviewStartValue() const
    {
        Target target;

        if ( m_isSet )
        {
            if ( m_isBoneID )
            {
                target = Target( m_previewStartBoneID );
            }
            else
            {
                target = Target( m_previewStartTransform );
            }
        }

        return target;
    }

    //-------------------------------------------------------------------------
    // Virtual Parameter
    //-------------------------------------------------------------------------

    VirtualParameterToolsNode::VirtualParameterToolsNode( String const& name, String const& categoryName )
        : m_name( name )
        , m_parameterCategory( categoryName )
    {
        EE_ASSERT( !m_name.empty() );
    }

    void VirtualParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );

        auto pParameterGraph = EE::New<FlowGraph>( GraphType::ValueTree );

        switch ( GetValueType() )
        {
            case GraphValueType::Bool:
            pParameterGraph->CreateNode<BoolResultToolsNode>();
            break;

            case GraphValueType::ID:
            pParameterGraph->CreateNode<IDResultToolsNode>();
            break;

            case GraphValueType::Int:
            pParameterGraph->CreateNode<IntResultToolsNode>();
            break;

            case GraphValueType::Float:
            pParameterGraph->CreateNode<FloatResultToolsNode>();
            break;

            case GraphValueType::Vector:
            pParameterGraph->CreateNode<VectorResultToolsNode>();
            break;

            case GraphValueType::Target:
            pParameterGraph->CreateNode<TargetResultToolsNode>();
            break;

            case GraphValueType::BoneMask:
            pParameterGraph->CreateNode<BoneMaskResultToolsNode>();
            break;

            case GraphValueType::Pose:
            pParameterGraph->CreateNode<PoseResultToolsNode>();
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }

        SetChildGraph( pParameterGraph );
    }

    int16_t VirtualParameterToolsNode::Compile( GraphCompilationContext& context ) const
    {
        auto const resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        EE_ASSERT( resultNodes.size() == 1 );

        auto pConnectedNode = resultNodes[0]->GetConnectedInputNode<FlowToolsNode>( 0 );
        if ( pConnectedNode == nullptr )
        {
            context.LogError( this, "Empty virtual parameter detected!" );
            return InvalidIndex;
        }

        return resultNodes[0]->GetConnectedInputNode<FlowToolsNode>( 0 )->Compile( context );
    }

    void VirtualParameterToolsNode::Rename( String const& name, String const& category )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_name = name;
        m_parameterCategory = category;
    }

    //-------------------------------------------------------------------------
    // Parameter Reference
    //-------------------------------------------------------------------------

    ParameterReferenceToolsNode::ParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        UpdateCachedParameterData();
    }

    ParameterReferenceToolsNode::ParameterReferenceToolsNode( ControlParameterToolsNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        UpdateCachedParameterData();
    }

    int16_t ParameterReferenceToolsNode::Compile( GraphCompilationContext& context ) const
    {
        return m_pParameter->Compile( context );
    }

    void ParameterReferenceToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        if ( auto pVP = GetReferencedVirtualParameter() )
        {
            pUserContext->NavigateTo( pVP->GetChildGraph() );
        }
    }

    void ParameterReferenceToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        bool const isPreviewing = pGraphNodeContext->HasDebugData();
        int16_t const runtimeNodeIdx = isPreviewing ? pGraphNodeContext->GetRuntimeGraphNodeIndex( m_pParameter->GetID() ) : InvalidIndex;

        //-------------------------------------------------------------------------

        BeginDrawInternalRegion( ctx, Color( 40, 40, 40 ) );

        if ( isPreviewing && ( runtimeNodeIdx != InvalidIndex ) && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
        {
            GraphValueType const valueType = m_pParameter->GetValueType();
            switch ( valueType )
            {
                case GraphValueType::Bool:
                {
                    auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<bool>( runtimeNodeIdx );
                    ImGui::Text( value ? "Value: True" : "Value: False" );
                }
                break;

                case GraphValueType::ID:
                {
                    auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<StringID>( runtimeNodeIdx );
                    if ( value.IsValid() )
                    {
                        ImGui::Text( "Value: %s", value.c_str() );
                    }
                    else
                    {
                        ImGui::Text( "Value: Invalid" );
                    }
                }
                break;

                case GraphValueType::Int:
                {
                    auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<int32_t>( runtimeNodeIdx );
                    ImGui::Text( "Value: %d", value );
                }
                break;

                case GraphValueType::Float:
                {
                    auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<float>( runtimeNodeIdx );
                    ImGui::Text( "Value: %.3f", value );
                }
                break;

                case GraphValueType::Vector:
                {
                    auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<Vector>( runtimeNodeIdx );
                    DrawVectorInfoText( ctx, value );
                }
                break;

                case GraphValueType::Target:
                {
                    auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<Target>( runtimeNodeIdx );
                    DrawTargetInfoText( ctx, value );
                }
                break;

                case GraphValueType::BoneMask:
                case GraphValueType::Pose:
                default:
                break;
            }
        }
        else
        {
            ImGui::NewLine();
        }

        EndDrawInternalRegion( ctx );
    }

    void ParameterReferenceToolsNode::SetReferencedParameter( FlowToolsNode* pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        EE_ASSERT( IsOfType<ControlParameterToolsNode>( pParameter ) || IsOfType<VirtualParameterToolsNode>( pParameter ) );
        EE_ASSERT( pParameter->GetValueType() == GetValueType() );
        m_pParameter = pParameter;
        UpdateCachedParameterData();
    }

    void ParameterReferenceToolsNode::UpdateCachedParameterData()
    {
        EE_ASSERT( m_pParameter != nullptr );

        if ( IsReferencingControlParameter() )
        {
            auto pParameter = GetReferencedControlParameter();
            m_parameterUUID = pParameter->GetID();
            m_parameterValueType = pParameter->GetValueType();
            m_parameterName = pParameter->GetParameterName();
            m_parameterCategory = pParameter->GetParameterCategory();
        }
        else
        {
            auto pParameter = GetReferencedVirtualParameter();
            m_parameterUUID = pParameter->GetID();
            m_parameterValueType = pParameter->GetValueType();
            m_parameterName = pParameter->GetParameterName();
            m_parameterCategory = pParameter->GetParameterCategory();
        }
    }

    String const& ParameterReferenceToolsNode::GetReferencedParameterCategory() const
    {
        if ( m_pParameter != nullptr )
        {
            if ( IsReferencingControlParameter() )
            {
                auto pParameter = GetReferencedControlParameter();
                return pParameter->GetParameterCategory();
            }
            else
            {
                auto pParameter = GetReferencedVirtualParameter();
                return pParameter->GetParameterCategory();
            }
        }

        return m_parameterCategory;
    }

    
}