#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ExternalGraphToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( ExternalGraphToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "External Graph"; }
        virtual char const* GetCategory() const override { return "Animation/Graphs"; }
        virtual bool IsPersistentNode() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual void PostPaste() override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

        String GetUniqueSlotName( String const& desiredName );
    private:

        EE_EXPOSE String                       m_name;
    };
}