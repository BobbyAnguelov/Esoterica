#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Widgets/Pickers/ResourcePickers.h"
#include "Base/Imgui/ImguiInputs.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API UITest
    {
    public:

        UITest( ToolsContext* pContext );

        void DrawWindow( bool* pIsWindowOpen );

    private:

        void DrawFonts();
        void DrawIconsInButtons();
        void DrawButtonsWithCalculatedWidth();
        void DrawSeparators();
        void DrawColoredButtons();
        void DrawTooltips();
        void DrawDropDownButtons();
        void DrawComboButtons();
        void DrawToggleButtons();
        void DrawInputText();
        void DrawInputCombo();
        void DrawIconButtons();
        void DrawNumericEditors();
        void DrawSpinnersAndAnimated();
        void DrawColorHelpers();
        void DrawColorCategories();
        void DrawResourceColors();
        void DrawHeadersAndSeparators();
        void DrawLayoutWidgets();
        void DrawSpecialWidgets();
        void DrawMessageBoxTests();
        void DrawDialogTests();
        void DrawResourcePickers();

    private:

        ToolsContext*                   m_pContext = nullptr;
        ResourcePicker                  m_picker;
        ResourcePicker                  m_compactPicker;
        ImGuiX::TextBuffer              m_buffer;
        ImGuiX::OptionData              m_optionData0;
        ImGuiX::OptionData              m_optionData1;

        ImGuiX::OptionData              m_comboData0;
        String                          m_selectedValue0;

        ImGuiX::OptionData              m_comboData1;
        String                          m_selectedValue1;
    };
}