#pragma once
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_UserContext.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_FlowGraph.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_StateMachineGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphCompilationContext;
    struct PoseNodeDebugInfo;

    //-------------------------------------------------------------------------

    enum class GraphType
    {
        EE_REGISTER_ENUM

        BlendTree,
        ValueTree,
        TransitionTree,
    };
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void DrawPoseNodeDebugInfo( VisualGraph::DrawContext const& ctx, float width, PoseNodeDebugInfo const& debugInfo );
    void DrawEmptyPoseNodeDebugInfo( VisualGraph::DrawContext const& ctx, float width );
    void DrawVectorInfoText( VisualGraph::DrawContext const& ctx, Vector const& vector );
    void DrawTargetInfoText( VisualGraph::DrawContext const& ctx, Target const& target );

    //-------------------------------------------------------------------------

    class FlowToolsNode : public VisualGraph::Flow::Node
    {
        EE_REGISTER_TYPE( FlowToolsNode );

    public:

        using VisualGraph::Flow::Node::Node;

        // Get the type of node this is, this is either the output type for the nodes with output or the first input for nodes with no outputs
        EE_FORCE_INLINE GraphValueType GetValueType() const
        {
            if ( GetNumOutputPins() > 0 )
            {
                return GraphValueType( GetOutputPin( 0 )->m_type );
            }
            else if ( GetNumInputPins() > 0 )
            {
                return GraphValueType( GetInputPin( 0 )->m_type );
            }
            else
            {
                return GraphValueType::Unknown;
            }
        }

        virtual ImColor GetTitleBarColor() const override;
        virtual ImColor GetPinColor( VisualGraph::Flow::Pin const& pin ) const override;

        // Get the types of graphs that this node is allowed to be placed in
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const = 0;

        // Is this node a persistent node i.e. is it always initialized 
        virtual bool IsPersistentNode() const { return false; }

        // Compile this node into its runtime representation. Returns the node index of the compiled node.
        virtual int16_t Compile( GraphCompilationContext& context ) const { return int16_t(); }

    protected:

        EE_FORCE_INLINE void CreateInputPin( char const* pPinName, GraphValueType pinType ) { VisualGraph::Flow::Node::CreateInputPin( pPinName, (uint32_t) pinType ); }
        EE_FORCE_INLINE void CreateOutputPin( char const* pPinName, GraphValueType pinType, bool allowMultipleOutputConnections = false ) { VisualGraph::Flow::Node::CreateOutputPin( pPinName, (uint32_t) pinType, allowMultipleOutputConnections ); }

        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) {}

        virtual bool IsActive( VisualGraph::UserContext* pUserContext ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, VisualGraph::Flow::Pin* pPin ) override;
    };
}