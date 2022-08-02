#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ExternalGraphEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( ExternalGraphEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual bool IsRenamable() const override { return true; }
        virtual void SetName( String const& newName ) override;

        virtual char const* GetTypeName() const override { return "External Graph"; }
        virtual char const* GetCategory() const override { return "Animation"; }
        virtual bool IsPersistentNode() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual void PostPaste() override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

        String GetUniqueSlotName( String const& desiredName );

    private:

        EE_EXPOSE String                       m_name;
    };
}