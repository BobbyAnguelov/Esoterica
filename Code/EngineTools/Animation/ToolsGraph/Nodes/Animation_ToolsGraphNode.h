#pragma once
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_UserContext.h"
#include "EngineTools/NodeGraph/NodeGraph_FlowGraph.h"

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
        TransitionConduit,
    };
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void DrawPoseNodeDebugInfo( NodeGraph::DrawContext const& ctx, float canvasWidth, PoseNodeDebugInfo const* pDebugInfo );
    void DrawRuntimeNodeIndex( NodeGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, NodeGraph::BaseNode* pNode, int16_t runtimeNodeIdx );
    void DrawVectorInfoText( NodeGraph::DrawContext const& ctx, Float3 const& vector );
    void DrawTargetInfoText( NodeGraph::DrawContext const& ctx, Target const& target );
    void DrawValueDisplayText( NodeGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, int16_t runtimeNodeIdx, GraphValueType valueType );

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API FlowToolsNode : public NodeGraph::FlowNode
    {
        EE_REFLECT_TYPE( FlowToolsNode );

    public:

        static inline StringID GetPinTypeForValueType( GraphValueType valueType ) { return GetIDForValueType( valueType ); }
        static inline GraphValueType GetValueTypeForPinType( StringID pinType ) { return GetValueTypeForID( pinType ); }

    public:

        using NodeGraph::FlowNode::FlowNode;

        virtual Color GetTitleBarColor() const override;
        virtual Color GetPinColor( NodeGraph::Pin const& pin ) const override;

        // Get the types of graphs that this node is allowed to be placed in
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const = 0;

        // Is this node a persistent node i.e. is it always initialized 
        virtual bool IsPersistentNode() const { return false; }

        // Compile this node into its runtime representation. Returns the node index of the compiled node.
        virtual int16_t Compile( GraphCompilationContext& context ) const { return int16_t(); }

        // Is this an anim clip reference node - i.e. a node that represents a singular anim clip
        virtual bool IsAnimationClipReferenceNode() const { return false; }

        // Get the graph value type for the output pin
        inline GraphValueType GetOutputValueType() const { return HasOutputPin() ? GetValueTypeForPinType( GetOutputPin( 0 )->m_type ) : GraphValueType::Unknown; }

        // IDs
        //-------------------------------------------------------------------------

        // Return any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const {}

        // Rename any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) {}

    protected:

        EE_FORCE_INLINE void CreateInputPin( char const* pPinName, GraphValueType pinType ) { NodeGraph::FlowNode::CreateInputPin( pPinName, GetPinTypeForValueType( pinType ) ); }
        EE_FORCE_INLINE void CreateOutputPin( char const* pPinName, GraphValueType pinType, bool allowMultipleOutputConnections = false ) { NodeGraph::FlowNode::CreateOutputPin( pPinName, GetPinTypeForValueType( pinType ), allowMultipleOutputConnections ); }

        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) {}

        virtual bool IsActive( NodeGraph::UserContext* pUserContext ) const override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual void DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, NodeGraph::Pin* pPin ) override;
    };
}