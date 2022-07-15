#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ZeroPoseEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( ZeroPoseEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Zero Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class ReferencePoseEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( ReferencePoseEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Reference Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class AnimationPoseEditorNode final : public DataSlotEditorNode
    {
        EE_REGISTER_TYPE( AnimationPoseEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Animation Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* const GetDefaultSlotName() const override { return "Pose"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override { return AnimationClip::GetStaticResourceTypeID(); }

    private:

        EE_EXPOSE FloatRange   m_inputTimeRemapRange = FloatRange( 0, 1 );
    };
}