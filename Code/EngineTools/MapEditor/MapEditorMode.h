#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/ToolsContext.h" 
#include "EngineTools/Entity/EntityEditor/EntityEditor_Context.h"
#include "EngineTools/PropertyGrid/PropertyGrid.h"
#include "Engine/UpdateContext.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API MapEditorMode : public IReflectedType
    {
        EE_REFLECT_TYPE( MapEditorMode );

        friend class MapEditor;

    public:

        MapEditorMode() = default;
        ~MapEditorMode();

        virtual char const* GetName() const = 0;

        virtual void Initialize( EditorContext* pEntityEditorContext );
        virtual void Shutdown();

        // Update this mode and draw all UI elements
        virtual void UpdateAndDraw( UpdateContext const& context, bool isFocused ) = 0;

        // Draw viewport overlay elements, return true to bypass map editor default overlay elements
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Viewport const* pViewport, bool isViewportHovered, bool isViewportFocused ) {}

        // Should any default map editor viewport interactions be allowed or does the edit mode have full control
        virtual bool AllowDefaultViewportInteractions() const { return true; }

        // Should any default map editor viewport overlay elements be drawn or does the edit mode have full control
        virtual bool AllowDefaultViewportOverlayElements() const { return true; }

    protected:

        virtual void PrePropertyGridChange( PropertyEditInfo const& info ) {}
        virtual void PostPropertyGridChange( PropertyEditInfo const& info ) {}

        // Get the world drawing context
        DebugDrawContext GetDebugDrawContext() { return m_pEntityEditorContext->GetDebugDrawContext(); }

        // Get the closest picking ID for this update
        PickingID GetPickingID() const { return m_pickingData.empty() ? PickingID() : m_pickingData.front(); }

        // Get all the picking data for this update
        PickingData const& GetPickingData() const { return m_pickingData; }

    protected:

        EditorContext* const                            m_pEntityEditorContext = nullptr;
        ToolsContext const* const                       m_pToolsContext = nullptr;
        PickingData                                     m_pickingData;

        PropertyGrid*                                   m_pPropertyGrid = nullptr;
        EventBindingID                                  m_prePropertyGridChangedEventID;
        EventBindingID                                  m_postPropertyGridChangedEventID;
    };
}