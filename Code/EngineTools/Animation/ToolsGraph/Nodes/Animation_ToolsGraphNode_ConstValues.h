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

        virtual char const* GetTypeName() const override { return "Bool"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() bool m_value = false;
    };

    //-------------------------------------------------------------------------

    class ConstIDToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstIDToolsNode );

    public:

        ConstIDToolsNode();

        virtual char const* GetTypeName() const override { return "ID"; }
        virtual char const* GetCategory() const override { return "Values/ID"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override { outIDs.emplace_back( m_value ); }
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override { if ( m_value == oldID ) { NodeGraph::ScopedNodeModification snm( this ); m_value = newID; } }

    private:

        EE_REFLECT( CustomEditor = "AnimGraph_ID" );
        StringID m_value;
    };

    //-------------------------------------------------------------------------

    class ConstFloatToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstFloatToolsNode );

    public:

        ConstFloatToolsNode();

        virtual char const* GetTypeName() const override { return "Float"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() float m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class ConstVectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstVectorToolsNode );

    public:

        ConstVectorToolsNode();

        virtual char const* GetTypeName() const override { return "Vector"; }
        virtual char const* GetCategory() const override { return "Values/Vector"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() Float3 m_value = Float3::Zero;
    };

    //-------------------------------------------------------------------------

    class ConstTargetToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstTargetToolsNode );

    public:

        ConstTargetToolsNode();

        virtual char const* GetTypeName() const override { return "Target"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() Transform m_value;
    };

    //-------------------------------------------------------------------------

    class ConstBoneTargetToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstBoneTargetToolsNode );

    public:

        ConstBoneTargetToolsNode();

        virtual char const* GetTypeName() const override { return "Target"; }
        virtual char const* GetCategory() const override { return "Values/BoneTarget"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() StringID m_boneName;
    };
}