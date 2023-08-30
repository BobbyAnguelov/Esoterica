#pragma once
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_UserContext.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_FlowGraph.h"

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
    void DrawPoseNodeDebugInfo( VisualGraph::DrawContext const& ctx, float canvasWidth, PoseNodeDebugInfo const* pDebugInfo );
    void DrawRuntimeNodeIndex( VisualGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, VisualGraph::BaseNode* pNode, int16_t runtimeNodeIdx );
    void DrawVectorInfoText( VisualGraph::DrawContext const& ctx, Vector const& vector );
    void DrawTargetInfoText( VisualGraph::DrawContext const& ctx, Target const& target );
    void DrawValueDisplayText( VisualGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, int16_t runtimeNodeIdx, GraphValueType valueType );

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API FlowToolsNode : public VisualGraph::Flow::Node
    {
        EE_REFLECT_TYPE( FlowToolsNode );

    public:

        static StringID const s_pinTypes[];
        static StringID GetPinTypeForValueType( GraphValueType valueType ) { return s_pinTypes[(uint8_t) valueType]; }
        static GraphValueType GetValueTypeForPinType( StringID pinType );

    public:

        using VisualGraph::Flow::Node::Node;

        virtual GraphValueType GetValueType() const = 0;
        virtual Color GetTitleBarColor() const override;
        virtual Color GetPinColor( VisualGraph::Flow::Pin const& pin ) const override;

        // Get the types of graphs that this node is allowed to be placed in
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const = 0;

        // Is this node a persistent node i.e. is it always initialized 
        virtual bool IsPersistentNode() const { return false; }

        // Compile this node into its runtime representation. Returns the node index of the compiled node.
        virtual int16_t Compile( GraphCompilationContext& context ) const { return int16_t(); }

        // Is this an anim clip reference node - i.e. a node that represents a singular anim clip
        virtual bool IsAnimationClipReferenceNode() const { return false; }

        // IDs
        //-------------------------------------------------------------------------

        // Return any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const {}

        // Rename any logic or event IDs entered into this node (things like event IDs, parameter ID values, etc...)
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) {}

    protected:

        EE_FORCE_INLINE void CreateInputPin( char const* pPinName, GraphValueType pinType ) { VisualGraph::Flow::Node::CreateInputPin( pPinName, GetPinTypeForValueType( pinType ) ); }
        EE_FORCE_INLINE void CreateOutputPin( char const* pPinName, GraphValueType pinType, bool allowMultipleOutputConnections = false ) { VisualGraph::Flow::Node::CreateOutputPin( pPinName, GetPinTypeForValueType( pinType ), allowMultipleOutputConnections ); }

        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) {}

        virtual bool IsActive( VisualGraph::UserContext* pUserContext ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, VisualGraph::Flow::Pin* pPin ) override;
    };
}