#include "EngineTools/Entity/EntityEditor/EntityEditor_ComponentTools.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Context.h"
#include "Engine/Render/Components/Component_Decals.h"
#include "Engine/Viewport/Viewport.h"
#include "Base/Imgui/MaterialDesignIcons.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    //-------------------------------------------------------------------------

    namespace DecalEditor
    {
        static Color const      s_neutralColor = Color( 66, 133, 244 );
        static float const      s_lineThickness = 4.0F;
    }

    //-------------------------------------------------------------------------

    class DecalComponentTools final : public EntityModel::TComponentTools<DecalComponent>
    {
    public:

        using TComponentTools::TComponentTools;

        virtual bool HasViewportSelectionWidget() const override { return true; }
        virtual char const* GetViewportSelectionWidgetIcon() const override { return EE_ICON_PROJECTOR; }
        virtual Color GetViewportSelectionWidgetColor() const override { return DecalEditor::s_neutralColor; }

        virtual bool DrawViewportEditor( EntityModel::EditorContext& context, Viewport const* pViewport, bool isFocused ) override
        {
            if ( pViewport == nullptr )
            {
                return false;
            }

            auto drawingCtx = context.GetDebugDrawContext();

            drawingCtx.DrawWireBox( m_pComponent->GetWorldBounds(), DecalEditor::s_neutralColor, DecalEditor::s_lineThickness, DebugDrawLayer::Screen );

            return false;
        }
    };

    //-------------------------------------------------------------------------

    EE_COMPONENT_TOOLS( DecalComponent, DecalComponentTools );
}
