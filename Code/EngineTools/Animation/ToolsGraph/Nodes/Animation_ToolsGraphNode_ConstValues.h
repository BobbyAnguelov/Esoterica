#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ConstBoolToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstBoolToolsNode );

    public:

        ConstBoolToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Bool"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() bool m_value;
    };

    //-------------------------------------------------------------------------

    class ConstIDToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstIDToolsNode );

    public:

        ConstIDToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::ID; }
        virtual char const* GetTypeName() const override { return "ID"; }
        virtual char const* GetCategory() const override { return "Values/ID"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override { outIDs.emplace_back( m_value ); }
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override { if ( m_value == oldID ) { VisualGraph::ScopedNodeModification snm( this ); m_value = newID; } }

    private:

        EE_REFLECT( "CustomEditor" : "AnimGraph_ID" );
        StringID m_value;
    };

    //-------------------------------------------------------------------------

    class ConstFloatToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstFloatToolsNode );

    public:

        ConstFloatToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() float m_value;
    };

    //-------------------------------------------------------------------------

    class ConstVectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstVectorToolsNode );

    public:

        ConstVectorToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Vector; }
        virtual char const* GetTypeName() const override { return "Vector"; }
        virtual char const* GetCategory() const override { return "Values/Vector"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() Float4 m_value;
    };

    //-------------------------------------------------------------------------

    class ConstTargetToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstTargetToolsNode );

    public:

        ConstTargetToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Target; }
        virtual char const* GetTypeName() const override { return "Target"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() Transform m_value;
    };
}