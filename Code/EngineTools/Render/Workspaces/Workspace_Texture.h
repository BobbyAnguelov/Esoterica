#pragma once

#include "EngineTools/Core/Workspaces/ResourceWorkspace.h"
#include "System/Render/RenderTexture.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class TextureWorkspace final : public TResourceWorkspace<Texture>
    {

    public:

        using TResourceWorkspace::TResourceWorkspace;

    private:

        virtual bool HasViewportWindow() const { return false; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

    private:

        String          m_previewWindowName;
        String          m_infoWindowName;
    };
}