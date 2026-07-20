#pragma once
#include "EngineTools/Core/EditorTool.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Utils/CategoryTree.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Settings;
    class SettingsRegistry;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API SystemSettingsEditorTool : public EditorTool
    {
    public:

        EE_SINGLETON_EDITOR_TOOL( SystemSettingsEditorTool );

    public:

        SystemSettingsEditorTool( ToolsContext const* pToolsContext );

        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_FORMAT_LIST_BULLETED_TYPE; }
        virtual bool SupportsMainMenu() const { return false; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    private:

        void DrawSettingsWindow( UpdateContext const& context, bool isFocused );

        void DrawSettingsCategory( Settings* pSettings, Category<TypeSystem::PropertyInfo const*> const& category );
        void DrawSettingsRow( Settings* pSettings, TypeSystem::PropertyInfo const* pPropertyInfo );

        ImGuiX::TextBuffer* GetScratchBuffer();
        void ResetScratchBufferFreeIndex() { m_scratchBufferFreeIdx = 0; }

    private:

        ImGuiX::FilterWidget            m_globalSettingsFilterWidget;
        SettingsRegistry*               m_pSettingsRegistry = nullptr;
        bool                            m_isVisible = false;
        TVector<ImGuiX::TextBuffer*>    m_scratchBuffers;
        int32_t                         m_scratchBufferFreeIdx = 0;
    };
}