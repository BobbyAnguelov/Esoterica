#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "Base/Render/RenderTexture.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class TextureEditor final : public TResourceEditor<Texture>
    {
        EE_EDITOR_TOOL( TextureEditor );

    public:

        using TResourceEditor::TResourceEditor;

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_IMAGE_OUTLINE; }
        virtual bool SupportsViewport() const { return false; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

        void DrawInfoWindow( UpdateContext const& context, bool isFocused );
        void DrawPreviewWindow( UpdateContext const& context, bool isFocused );
    };
}