#pragma once
#include "EngineTools/MapEditor/MapEditorMode.h"

//-------------------------------------------------------------------------

namespace EE
{
    // This is a developer only mode for us to use to test certain systems or functions of the engine
    // It is intended for quick hacking and so will likely end up becoming a big garbage dump of stuff over time
    class EE_ENGINETOOLS_API DeveloperToolsMapEditorMode final : public EntityModel::MapEditorMode
    {
        EE_REFLECT_TYPE( DeveloperToolsMapEditorMode );

        enum class Mode
        {
            None,
            DebugMeshTests,
            IntersectionTests
        };

    public:

        DeveloperToolsMapEditorMode() = default;
        ~DeveloperToolsMapEditorMode();

        virtual char const* GetName() const override { return "Developer Tools"; }
        virtual void UpdateAndDraw( UpdateContext const& context, bool isFocused ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Viewport const* pViewport, bool isViewportHovered, bool isViewportFocused ) override;
        virtual bool AllowDefaultViewportInteractions() const override;
        virtual bool AllowDefaultViewportOverlayElements() const override;
        virtual void Initialize( EntityModel::EditorContext* pEntityEditorContext ) override;
        virtual void Shutdown() override;

    private:

        // Debug Meshes
        //-------------------------------------------------------------------------

        void UpdateAndDraw_DebugMeshTests( UpdateContext const& context, bool isFocused );
        void DrawViewportOverlayElements_DebugMeshTests( UpdateContext const& context, Viewport const* pViewport );

        // Intersections
        //-------------------------------------------------------------------------

        void UpdateAndDraw_IntersectionTests( UpdateContext const& context, bool isFocused );

    private:

        Mode                        m_mode = Mode::IntersectionTests;
    };
}