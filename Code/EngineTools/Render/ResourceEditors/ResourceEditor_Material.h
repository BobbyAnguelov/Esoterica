#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshComponent;

    //-------------------------------------------------------------------------

    class MaterialEditor final : public TResourceEditor<Material>
    {
        EE_EDITOR_TOOL( MaterialEditor );

    public:

        using TResourceEditor::TResourceEditor;

    private:

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