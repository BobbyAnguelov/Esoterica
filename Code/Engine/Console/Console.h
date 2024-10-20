#pragma once
#include "Engine/_Module/API.h"
#include "Base/Systems.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE { class UpdateContext; }
namespace EE::Settings { class SettingsRegistry; }

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class EE_ENGINE_API Console final : public ISystem
    {
    public:

        EE_SYSTEM( Console );

    public:

        Console() = default;

        void Initialize( Settings::SettingsRegistry& settingsRegistry );
        void Shutdown();

        inline bool IsVisible() const { return m_isVisible; }
        inline void SetVisible( bool isVisible ) { m_isVisible = isVisible; }

        void Update( UpdateContext const& context );

    private:

        void DrawGlobalSettingsEditor();
        void DrawWorldSettingsEditor();

    private:

        ImGuiX::FilterWidget            m_globalSettingsFilterWidget;
        ImGuiX::FilterWidget            m_worldSettingsFilterWidget;
        Settings::SettingsRegistry*     m_pSettingsRegistry = nullptr;
        bool                            m_isVisible = false;
    };
}
#endif