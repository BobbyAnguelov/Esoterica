#pragma once

#include "EngineTools/Core/Workspace.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class CollisionMeshWorkspace final : public TWorkspace<CollisionMesh>
    {
    public:

        using TWorkspace::TWorkspace;

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_PALETTE_SWATCH; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual bool HasViewportToolbar() const { return true; }
    };
}