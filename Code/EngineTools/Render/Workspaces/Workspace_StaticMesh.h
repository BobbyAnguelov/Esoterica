#pragma once

#include "EngineTools/Core/Workspace.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshWorkspace final : public TWorkspace<StaticMesh>
    {

    public:

        using TWorkspace::TWorkspace;

    private:

        virtual char const* GetWorkspaceUniqueTypeName() const override { return "Static Mesh"; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void Update( UpdateContext const& context, bool isFocused ) override;
        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_PINE_TREE; }

        void DrawInfoWindow( UpdateContext const& context, bool isFocused );

    private:

        Entity*         m_pPreviewEntity = nullptr;
        bool            m_showOrigin = true;
        bool            m_showBounds = true;
        bool            m_showVertices = false;
        bool            m_showNormals = false;
    };
}