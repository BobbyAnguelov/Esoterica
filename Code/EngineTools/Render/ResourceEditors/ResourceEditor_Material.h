#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "Engine/Render/RenderMaterial.h"
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
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual bool HasViewportToolbar() const { return true; }

        void CreatePreviewEntity();
        void DestroyPreviewEntity();

    private:

        Entity*                 m_pPreviewEntity = nullptr;
        StaticMeshComponent*    m_pPreviewComponent = nullptr;
    };
}