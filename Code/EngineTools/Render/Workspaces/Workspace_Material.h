#pragma once

#include "EngineTools/Core/Workspace.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshComponent;

    //-------------------------------------------------------------------------

    class MaterialWorkspace final : public TWorkspace<Material>
    {
    public:

        using TWorkspace::TWorkspace;

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual bool HasViewportToolbar() const { return true; }

    private:

        StaticMeshComponent*    m_pPreviewComponent = nullptr;
        String                  m_materialDetailsWindowName;
    };
}