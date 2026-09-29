#include "Game/Base/Components/Component_SpawnPoint.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_ComponentTools.h"
#include "Base/Imgui/EsotericaIcons.h"

//-------------------------------------------------------------------------

namespace EE
{
    class SpawnPointComponentTools : public EntityModel::TComponentTools<SpawnPointComponent>
    {
    public:

        using TComponentTools::TComponentTools;

        virtual bool HasViewportSelectionWidget() const override { return true; }
        char const* GetViewportSelectionWidgetIcon() const override { return EE_ICON_EDITOR_SPAWNPOINT; }
        Color GetViewportSelectionWidgetColor() const override { return Colors::Cyan; }
    };

    EE_COMPONENT_TOOLS( SpawnPointComponent, SpawnPointComponentTools );
}
