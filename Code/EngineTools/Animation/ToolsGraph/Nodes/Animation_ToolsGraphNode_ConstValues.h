#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ConstBoolToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstBoolToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Bool"; }
        virtual char const* GetCategory() const override { return "Values/Const"; }
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

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::ID; }
        virtual char const* GetTypeName() const override { return "ID"; }
        virtual char const* GetCategory() const override { return "Values/Const"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override { outIDs.emplace_back( m_value ); }

    private:

        EE_REFLECT() StringID m_value;
    };

    //-------------------------------------------------------------------------
    
    class ConstIntToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstIntToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Int; }
        virtual char const* GetTypeName() const override { return "Int"; }
        virtual char const* GetCategory() const override { return "Values/Const"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() int32_t m_value;
    };

    //-------------------------------------------------------------------------

    class ConstFloatToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ConstFloatToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Float"; }
        virtual char const* GetCategory() const override { return "Values/Const"; }
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

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Vector; }
        virtual char const* GetTypeName() const override { return "Vector"; }
        virtual char const* GetCategory() const override { return "Values/Const"; }
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

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Target; }
        virtual char const* GetTypeName() const override { return "Target"; }
        virtual char const* GetCategory() const override { return "Values/Const"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() Transform m_value;
    };
}