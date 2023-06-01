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
        EE_REFLECT_ENUM

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
    void DrawValueDisplayText( VisualGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, int16_t runtimeNodeIdx, GraphValueType valueType );

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API FlowToolsNode : public VisualGraph::Flow::Node
    {
        EE_REFLECT_TYPE( FlowToolsNode );

    public:

        using VisualGraph::Flow::Node::Node;

        virtual GraphValueType GetValueType() const = 0;
        virtual ImColor GetTitleBarColor() const override;
        virtual ImColor GetPinColor( VisualGraph::Flow::Pin const& pin ) const override;

        // Get the types of graphs that this node is allowed to be placed in
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const = 0;

        // Is this node a persistent node i.e. is it always initialized 
        virtual bool IsPersistentNode() const { return false; }

        // Compile this node into its runtime representation. Returns the node index of the compiled node.
        virtual int16_t Compile( GraphCompilationContext& context ) const { return int16_t(); }

        // Return any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const {}

    protected:

        EE_FORCE_INLINE void CreateInputPin( char const* pPinName, GraphValueType pinType ) { VisualGraph::Flow::Node::CreateInputPin( pPinName, (uint32_t) pinType ); }
        EE_FORCE_INLINE void CreateOutputPin( char const* pPinName, GraphValueType pinType, bool allowMultipleOutputConnections = false ) { VisualGraph::Flow::Node::CreateOutputPin( pPinName, (uint32_t) pinType, allowMultipleOutputConnections ); }

        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) {}

        virtual bool IsActive( VisualGraph::UserContext* pUserContext ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, VisualGraph::Flow::Pin* pPin ) override;
    };
}