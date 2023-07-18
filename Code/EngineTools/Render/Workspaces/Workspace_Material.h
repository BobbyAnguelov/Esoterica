#pragma once

#include "EngineTools/Core/Workspace.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Base/Imgui/ImguiX.h"

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

        virtual char const* GetDockingUniqueTypeName() const override { return "Material"; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_PALETTE_SWATCH; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual bool HasViewportToolbar() const { return true; }

        void DrawDetailsWindow( UpdateContext const& context, bool isFocused );

    private:

        StaticMeshComponent*    m_pPreviewComponent = nullptr;
    };
}