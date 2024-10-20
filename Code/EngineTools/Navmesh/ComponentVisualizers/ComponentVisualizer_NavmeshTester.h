#pragma once
#include "EngineTools/Entity/ComponentVisualizer.h"
#include "Base/Imgui/ImguiGizmo.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshTesterComponentVisualizer : public ComponentVisualizer
    {
        EE_REFLECT_TYPE( NavmeshTesterComponentVisualizer );

    public:

        virtual TypeSystem::TypeID GetSupportedType() const override;
        virtual bool HasToolbar() const override { return true; }
        virtual void DrawToolbar( ComponentVisualizerContext const& context ) override;
        virtual void UpdateVisualizedComponent( EntityComponent* pComponent ) override;
        virtual void Visualize( ComponentVisualizerContext const& context ) override;

    private:

        class NavmeshTesterComponent    *m_pTesterComponent = nullptr;
        ImGuiX::Gizmo                   m_startGizmo;
        Transform                       m_startTransform;
        ImGuiX::Gizmo                   m_endGizmo;
        Transform                       m_endTransform;
    };
}