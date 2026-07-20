#pragma once
#include "EngineTools/MapEditor/MapEditorMode.h"
#include "EngineTools/PropertyGrid/PropertyGrid.h"
#include "Engine/Physics/PhysicsQuery.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    // This is a developer only mode for us to use to test certain systems or functions of the engine
    // It is intended for quick hacking and so will likely end up becoming a big garbage dump of stuff over time
    class EE_ENGINETOOLS_API PhysicsMapEditorMode final : public EntityModel::MapEditorMode
    {
        EE_REFLECT_TYPE( PhysicsMapEditorMode );

        enum class ShapeType
        {
            EE_REFLECT_ENUM

            Ray, // Has to be first so it can easily be skipped

            Box,
            Capsule,
            Cylinder,
            Sphere,
        };

        // Needed for the custom property editor
        struct Settings : public IReflectedType
        {
            EE_REFLECT_TYPE( Settings );

            EE_REFLECT();
            CollisionSettings           m_collisionSettings;

            EE_REFLECT( Hidden );
            ShapeType                   m_shapeType = ShapeType::Ray;

            EE_REFLECT( Hidden );
            Float3                      m_extents = Float3( 0.25f, 0.25f, 0.25f );

            EE_REFLECT( Hidden );
            Transform                   m_sweepStart;

            EE_REFLECT( Hidden );
            Transform                   m_sweepEnd;

            EE_REFLECT( Hidden );
            bool                        m_isCastQuery = true;
        };

    public:

        PhysicsMapEditorMode() = default;
        ~PhysicsMapEditorMode() = default;

        virtual char const* GetName() const override { return "Physics"; }
        virtual void UpdateAndDraw( UpdateContext const& context, bool isFocused ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Viewport const* pViewport, bool isViewportHovered, bool isViewportFocused ) override;
        virtual bool AllowDefaultViewportInteractions() const override { return !m_isInteractingWithPhysicsGizmo; }
        virtual bool AllowDefaultViewportOverlayElements() const override { return false; }
        virtual void Initialize( EntityModel::EditorContext* pEntityEditorContext ) override;
        virtual void Shutdown() override;

    private:

        void DrawBoxSettings( UpdateContext const& context, bool isFocused );
        void DrawSphereSettings( UpdateContext const& context, bool isFocused );
        void DrawCapsuleAndCylinderSettings( UpdateContext const& context, bool isFocused );
        void UpdateAndDrawShapeCast( UpdateContext const& context, bool isFocused );
        void UpdateAndDrawOverlap( UpdateContext const& context, bool isFocused );

        String CopyTransformsToString() const;
        void PasteTransformsFromString( String const& str );

    private:

        PropertyGrid*               m_pPropertyGrid = nullptr;
        ImGuiX::Gizmo               m_startGizmo;
        ImGuiX::Gizmo               m_endGizmo;

        Settings                    m_settings;

        bool                        m_isInteractingWithPhysicsGizmo = false;
        CastQuery                   m_castQuery;
        OverlapQuery                m_overlapQuery;

    };
}