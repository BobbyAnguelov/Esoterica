#pragma once
#include "Animation_ToolsGraphNode_DataSlot.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ZeroPoseToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ZeroPoseToolsNode );

    public:

        ZeroPoseToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Zero Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class ReferencePoseToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ReferencePoseToolsNode );

    public:

        ReferencePoseToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Reference Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class AnimationPoseToolsNode final : public DataSlotToolsNode
    {
        EE_REFLECT_TYPE( AnimationPoseToolsNode );

    public:

        AnimationPoseToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Animation Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool DrawPinControls( VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin ) override;

        virtual char const* GetDefaultSlotName() const override { return "Pose"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override { return AnimationClip::GetStaticResourceTypeID(); }
        virtual bool IsDragAndDropTargetForResourceType( ResourceTypeID typeID ) const override { return false; }

    private:

        // Use this to remap an input value into a valid 0~1 range.
        EE_REFLECT();
        FloatRange              m_inputTimeRemapRange;

        // User specified fixed time
        EE_REFLECT();
        float                   m_fixedTimeValue = 0.0f;

        // Whether the input is in frames or percentage
        EE_REFLECT();
        bool                    m_useFramesAsInput = false;
    };
}