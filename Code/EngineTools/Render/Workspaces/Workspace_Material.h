#pragma once

#include "EngineTools/Core/Workspaces/ResourceWorkspace.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshComponent;

    //-------------------------------------------------------------------------

    class MaterialWorkspace final : public TResourceWorkspace<Material>
    {
    public:

        using TResourceWorkspace::TResourceWorkspace;

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual bool HasViewportToolbar() const { return true; }

    private:

        StaticMeshComponent*    m_pPreviewComponent = nullptr;
        String                  m_materialDetailsWindowName;
    };
}