#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "Engine/Render/RenderTexture.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class TextureEditor final : public TResourceEditor<TextureResource>
    {
        EE_EDITOR_TOOL( TextureEditor );

    public:

        using TResourceEditor::TResourceEditor;

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_IMAGE_OUTLINE; }
        virtual bool SupportsViewport() const override { return false; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport ) override;

        void DrawPreviewWindow( UpdateContext const& context, bool isFocused );

    private:

        RHI::Sampler* m_pPointWrapSampler = nullptr;
    };
}
