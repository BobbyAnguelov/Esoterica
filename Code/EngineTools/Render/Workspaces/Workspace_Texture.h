#pragma once

#include "EngineTools/Core/Workspace.h"
#include "System/Render/RenderTexture.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class TextureWorkspace final : public TWorkspace<Texture>
    {

    public:

        using TWorkspace::TWorkspace;

    private:

        virtual char const* GetWorkspaceUniqueTypeName() const override { return "Texture"; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_IMAGE_OUTLINE; }
        virtual bool HasViewportWindow() const { return false; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

        void DrawInfoWindow( UpdateContext const& context, bool isFocused );
        void DrawPreviewWindow( UpdateContext const& context, bool isFocused );
    };
}