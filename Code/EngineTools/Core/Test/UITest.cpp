#include "UITest.h"
#include "EngineTools/Core/SystemDialogs.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/Render/RenderMaterial.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE
{
    UITest::UITest( ToolsContext* pContext )
        : m_pContext( pContext )
        , m_picker( *pContext, Render::Material::GetStaticResourceTypeID() )
        , m_compactPicker( *pContext, Render::Material::GetStaticResourceTypeID() )
        , m_buffer( 8 )
    {
        m_compactPicker.SetCompactMode( true );

        //-------------------------------------------------------------------------

        TVector<ImGuiX::OptionData::Option> const choices =
        {
            ImGuiX::OptionData::Option( "catnip" ),
            ImGuiX::OptionData::Option( "cats" ),
            ImGuiX::OptionData::Option( "carrots" ),
            ImGuiX::OptionData::Option( "dogs" ),
            ImGuiX::OptionData::Option( "ducks" ),
            ImGuiX::OptionData::Option( "rabbits" ),
            ImGuiX::OptionData::Option( "ragu" ),
            ImGuiX::OptionData::Option( "turtles" ),
            ImGuiX::OptionData::Option( "catnip2", "2" ),
            ImGuiX::OptionData::Option( "cats2", "2" ),
            ImGuiX::OptionData::Option( "carrots2", "2" ),
            ImGuiX::OptionData::Option( "dogs2", "2" ),
            ImGuiX::OptionData::Option( "ducks2", "2" ),
            ImGuiX::OptionData::Option( "rabbits2", "2" ),
            ImGuiX::OptionData::Option( "ragu2", "2" ),
            ImGuiX::OptionData::Option( "turtles2", "2" ),
            ImGuiX::OptionData::Option( "catnip3", "3" ),
            ImGuiX::OptionData::Option( "cats3", "3" ),
            ImGuiX::OptionData::Option( "carrots3", "3" ),
            ImGuiX::OptionData::Option( "dogs3", "3" ),
            ImGuiX::OptionData::Option( "ducks3", "3" ),
            ImGuiX::OptionData::Option( "rabbits3", "3" ),
            ImGuiX::OptionData::Option( "ragu3", "3" ),
            ImGuiX::OptionData::Option( "turtles3", "3" ),
            ImGuiX::OptionData::Option( "catnip4", "4" ),
            ImGuiX::OptionData::Option( "cats4", "4" ),
            ImGuiX::OptionData::Option( "carrots4", "4" ),
            ImGuiX::OptionData::Option( "dogs4", "4" ),
            ImGuiX::OptionData::Option( "ducks4", "4" ),
            ImGuiX::OptionData::Option( "rabbits4", "4" ),
            ImGuiX::OptionData::Option( "ragu4" , "4" ),
            ImGuiX::OptionData::Option( "turtles4" , "4" ),
        };

        m_optionData0.SetOptions( choices );

        auto OptionProvider = [] ( TVector<ImGuiX::OptionData::Option>& options )
        {
            options =
            {
                ImGuiX::OptionData::Option( "moo" ),
                ImGuiX::OptionData::Option( "cow", "Animals" ),
                ImGuiX::OptionData::Option( "woof" ),
                ImGuiX::OptionData::Option( "Dog", "Animals" ),
            };
        };

        m_optionData1.SetOptionProvider( OptionProvider );

        //-------------------------------------------------------------------------

        m_comboData0.m_options.emplace_back( "A", "Alpha" );
        m_comboData0.m_options.emplace_back( "B", "Alpha" );
        m_comboData0.m_options.emplace_back( "C", "Alpha" );

        m_comboData0.m_options.emplace_back( "1", "Numeric" );
        m_comboData0.m_options.emplace_back( "2", "Numeric" );
        m_comboData0.m_options.emplace_back( "3", "Numeric" );

        //-------------------------------------------------------------------------

        auto DrawPreviewFn = [] ( ImGuiX::OptionData const* pData, ImGuiX::OptionData::Option const* pSelectedOption )
        {
            if ( pSelectedOption != nullptr )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, Color( (uint32_t) pSelectedOption->m_userData ) );
                ImGui::TextUnformatted( pSelectedOption->m_text.c_str() );
                ImGui::PopStyleColor();
            }
        };

        auto DrawFilteredOptionFn = [] ( ImGuiX::OptionData const* pData, int32_t nfilteredIdx )
        {
            ImGuiX::OptionData::Option const& filteredOption = pData->m_filteredOptions[nfilteredIdx];
            EE_ASSERT( filteredOption.IsValid() );

            bool isSelected = false;

            ImGui::PushID( nfilteredIdx );
            ImGui::PushStyleColor( ImGuiCol_Text, Color( (uint32_t) filteredOption.m_userData ) );
            isSelected = ImGui::Selectable( filteredOption.m_text.c_str() );
            ImGui::PopStyleColor();
            ImGui::PopID();

            return isSelected;
        };

        m_comboData1.m_drawPreviewFunc = DrawPreviewFn;
        m_comboData1.m_drawFilteredOptionFunc = DrawFilteredOptionFn;

        m_comboData1.m_options.emplace_back( "A", "Alpha", Colors::Red.ToUInt32() );
        m_comboData1.m_options.emplace_back( "B", "Alpha", Colors::Green.ToUInt32() );
        m_comboData1.m_options.emplace_back( "C", "Alpha", Colors::Blue.ToUInt32() );

        m_comboData1.m_options.emplace_back( "1", "Numeric", Colors::Teal.ToUInt32() );
        m_comboData1.m_options.emplace_back( "2", "Numeric", Colors::Cyan.ToUInt32() );
        m_comboData1.m_options.emplace_back( "3", "Numeric", Colors::Magenta.ToUInt32() );
    }

    //-------------------------------------------------------------------------

    void UITest::DrawFonts()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Fonts" ) )
        {
            ImGui::SeparatorText( "Mixed Fonts" );

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Tiny );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Tiny" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::TinyItalic);
                ImGui::Text( EE_ICON_ALERT"This is a test - Tiny Italic" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::TinyBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Tiny Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::TinyBoldItalic );
                ImGui::Text( EE_ICON_ALERT"This is a test - Tiny Bold Italic" );
            }

            
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Small" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallItalic );
                ImGui::Text( EE_ICON_ALERT"This is a test - Small Italic" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Small Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBoldItalic );
                ImGui::Text( EE_ICON_ALERT"This is a test - Small Bold Italic" );
            }


            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Medium" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumItalic );
                ImGui::Text( EE_ICON_ALERT"This is a test - Medium Italic" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Medium Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBoldItalic );
                ImGui::Text( EE_ICON_ALERT"This is a test - Medium Bold Italic" );
            }


            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Large" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeItalic );
                ImGui::Text( EE_ICON_ALERT"This is a test - Large Italic" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Large Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBoldItalic );
                ImGui::Text( EE_ICON_ALERT"This is a test - Large Bold Italic" );
            }


            {
                ImGuiX::ScopedFont sf( ImGuiX::FontType::Regular, 35 );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Regular Custom Size 35" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::FontType::BoldItalic, 23 );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - BoldItalic Custom Size 23" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::FontType::Bold, 76, Colors::HotPink );
                ImGui::Text( EE_ICON_ALERT"This is a test - Pink BoldItalic Custom Size 64" );
            }
        }
    }

    void UITest::DrawIconsInButtons()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Icons In Buttons" ) )
        {
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                ImGui::Button( EE_ICON_HAIR_DRYER );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_Z_WAVE );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_KANGAROO );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_YIN_YANG );
            }

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                ImGui::Button( EE_ICON_HAIR_DRYER );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_Z_WAVE );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_KANGAROO );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_YIN_YANG );
            }

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                ImGui::Button( EE_ICON_HAIR_DRYER );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_Z_WAVE );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_KANGAROO );
                ImGui::SameLine();
                ImGui::Button( EE_ICON_YIN_YANG );
            }
        }
    }

    void UITest::DrawButtonsWithCalculatedWidth()
    {
        if ( ImGui::CollapsingHeader( "Buttons with Calculated Widths" ) )
        {
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                float const w0 = ImGuiX::CalculateButtonWidth( EE_ICON_HAIR_DRYER );
                ImGui::Button( EE_ICON_HAIR_DRYER, ImVec2( w0, 0 ) );

                float const w1 = ImGuiX::CalculateButtonWidth( EE_ICON_CAR_2_PLUS" TEST" );
                ImGui::Button( EE_ICON_CAR_2_PLUS" TEST", ImVec2( w1, 0));
            }

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                float const w0 = ImGuiX::CalculateButtonWidth( EE_ICON_HAIR_DRYER );
                ImGui::Button( EE_ICON_HAIR_DRYER, ImVec2( w0, 0 ) );

                float const w1 = ImGuiX::CalculateButtonWidth( EE_ICON_CAR_2_PLUS" TEST" );
                ImGui::Button( EE_ICON_CAR_2_PLUS" TEST", ImVec2( w1, 0 ) );
            }

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                float const w0 = ImGuiX::CalculateButtonWidth( EE_ICON_HAIR_DRYER );
                ImGui::Button( EE_ICON_HAIR_DRYER, ImVec2( w0, 0 ) );

                float const w1 = ImGuiX::CalculateButtonWidth( EE_ICON_CAR_2_PLUS" TEST" );
                ImGui::Button( EE_ICON_CAR_2_PLUS" TEST", ImVec2( w1, 0 ) );
            }
        }
    }

    void UITest::DrawSeparators()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Separators" ) )
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Start Test" );
            ImGuiX::SameLineSeparator( 20 );

            ImGui::SameLine( 0, 0 );
            ImGui::Text( "Test" );

            ImGui::SameLine( 0, 0 );
            ImGuiX::SameLineSeparator();

            ImGui::SameLine( 0, 0 );
            ImGui::Text( "Test" );

            ImGui::SameLine( 0, 0 );
            ImGuiX::SameLineSeparator( 40 );

            ImGui::SameLine( 0, 0 );
            ImGui::Text( "Test" );

            ImGui::SameLine( 0, 0 );
            ImGuiX::SameLineSeparator();

            ImGui::Text( "End Test" );
        }
    }

    void UITest::DrawColoredButtons()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Colored Buttons" ) )
        {
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                ImGuiX::ButtonColored( EE_ICON_PLUS"ADD", Colors::Green, Colors::White );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                ImGuiX::ButtonColored( EE_ICON_PLUS"ADD", Colors::Green, Colors::White );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                ImGuiX::ButtonColored( EE_ICON_PLUS"ADD", Colors::Green, Colors::White );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                ImGuiX::ButtonColored( EE_ICON_PLUS"ADD", Colors::Green, Colors::White );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                ImGuiX::ButtonColored( EE_ICON_PLUS"ADD", Colors::Green, Colors::White );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                ImGuiX::ButtonColored( EE_ICON_PLUS"ADD", Colors::Green, Colors::White );
            }
        }
    }

    void UITest::DrawTooltips()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Tooltips" ) )
        {
            ImGui::Text( "Some Text" );
            ImGuiX::TextTooltip( "Text Tooltip - no delay since GImGui->HoveredIdTimer is not updated for text" );

            ImGui::Button( "Button A" );
            ImGuiX::ItemTooltip( "Item Tooltip" );
        }
    }

    void UITest::DrawDropDownButtons()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Drop Down Buttons" ) )
        {
            auto TestCombo = []
            {
                ImGui::Text( "Test" );
            };

            ImGuiX::DropDownIconButton( EE_ICON_TEST_TUBE, "Test Width: 0", TestCombo, Colors::RoyalBlue, ImVec2( 0, 0 ) );
            ImGuiX::DropDownIconButton( EE_ICON_TEST_TUBE, "Test Width: 100", TestCombo, Colors::RoyalBlue, ImVec2( 100, 0 ) );
            ImGui::SameLine();
            ImGuiX::DropDownIconButton( EE_ICON_TEST_TUBE, "Test Width: 200", TestCombo, Colors::RoyalBlue, ImVec2( 200, 0 ) );
            ImGuiX::DropDownIconButton( EE_ICON_TEST_TUBE, "Test Width: 1000", TestCombo, Colors::RoyalBlue, ImVec2( 1000, 0 ) );
            ImGuiX::DropDownIconButton( EE_ICON_TEST_TUBE, "Test Width: -1 ", TestCombo, Colors::RoyalBlue, ImVec2( -1, 0 ) );
        }
    }

    void UITest::DrawComboButtons()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Combo Buttons" ) )
        {
            auto TestCombo = []
            {
                ImGui::Text( "Test" );
            };

            ImGuiX::ComboButton( EE_ICON_BUG" Test Combo Width: -1", TestCombo, ImVec2( -1, 0 ) );

            ImGuiX::ComboIconButton( EE_ICON_CAR, "Test Combo Width: 0", TestCombo, Colors::Green, ImVec2( 0, 40 ) );
            ImGui::SameLine();
            ImGuiX::ComboIconButton( EE_ICON_COW, "Test Combo Sameline", TestCombo, Colors::RoyalBlue, ImVec2( 0, 40 ) );

            ImGuiX::ComboIconButtonColored( EE_ICON_DOG, "Test Play 2", TestCombo, Colors::Green, Colors::OrangeRed, Colors::Yellow, ImVec2( 200, 0 ), false );
        }
    }

    void UITest::DrawToggleButtons()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Toggle Buttons" ) )
        {
            static bool toggle = false;
            ImGuiX::ToggleButton( EE_ICON_TOGGLE_SWITCH" Toggle Button On", EE_ICON_TOGGLE_SWITCH_OFF" Toggle Button Off", toggle, ImVec2( -1, 0 ) );

            ImGuiX::FlatToggleButton( EE_ICON_TOGGLE_SWITCH, "Toggle Button On", EE_ICON_TOGGLE_SWITCH_OFF, "Toggle Button Off", toggle, ImVec2( -1, 0 ) );

            ImGuiX::FlatToggleButton( "Toggle Button On###3", "Toggle Button Off###3", toggle, ImVec2( -1, 0 ) );
        }
    }

    void UITest::DrawInputText()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Input Text" ) )
        {
            static char buffer[255];

            //-------------------------------------------------------------------------

            ImGuiX::InputTextWithClearButton( "ITCB0", "width: 0", buffer, 255 );

            ImGui::SetNextItemWidth( 200 );
            ImGuiX::InputTextWithClearButton( "ITCB1", "width: 200", buffer, 255 );

            ImGui::SetNextItemWidth( -1 );
            ImGuiX::InputTextWithClearButton( "ITCB2", "width: -1 ", buffer, 255 );

            //-------------------------------------------------------------------------

            auto Callback = [] ( ImGuiInputTextCallbackData* pData ) -> int
            {
                UITest* pCtx = (UITest*) pData->UserData;

                if ( pData->EventFlag == ImGuiInputTextFlags_CallbackResize )
                {
                    pCtx->m_buffer.Resize( pData->BufTextLen );
                    pData->Buf = pCtx->m_buffer.Data();
                }

                return 0;
            };

            ImGui::SetNextItemWidth( 200 );
            ImGuiX::InputTextWithOptions( "ITWO0", m_buffer.Data(), m_buffer.Size(), &m_optionData0, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_EnterReturnsTrue, Callback, this );

            ImGui::SetNextItemWidth( -1 );
            ImGuiX::InputTextWithOptions( "ITWO1", buffer, 255, &m_optionData1 );
        }
    }

    void UITest::DrawInputCombo()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Input Combo" ) )
        {
            UUID selectedID0 = m_comboData0.FindItemIDByText( m_selectedValue0 );
            ImGui::SetNextItemWidth( 350 );
            if ( ImGuiX::ComboWithFilter( "CMB0", &m_comboData0, selectedID0 ) )
            {
                auto pOption = m_comboData0.TryGetOption( selectedID0 );
                m_selectedValue0 = ( pOption != nullptr ) ? pOption->m_text : "";
            }

            UUID selectedID1 = m_comboData1.FindItemIDByText( m_selectedValue1 );
            ImGui::SetNextItemWidth( -1 );
            if ( ImGuiX::ComboWithFilter( "CMB1", &m_comboData1, selectedID1 ) )
            {
                auto pOption = m_comboData1.TryGetOption( selectedID1 );
                m_selectedValue1 = ( pOption != nullptr ) ? pOption->m_text : "";
            }
        }
    }

    void UITest::DrawIconButtons()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Icon Buttons" ) )
        {
            ImGuiX::IconButton( EE_ICON_KANGAROO, "Test - Auto", Colors::PaleGreen );

            ImGuiX::IconButton( EE_ICON_HOME, "Home - Auto", Colors::RoyalBlue );

            ImGuiX::IconButton( EE_ICON_MOVIE_PLAY, "Play ( 0, 80 )", Colors::LightPink, ImVec2( 0, 80 ) );

            ImGuiX::IconButton( EE_ICON_KANGAROO, "Test (60,20)", Colors::PaleGreen, ImVec2( 60, 20 ) );

            ImGuiX::IconButton( EE_ICON_HOME, "Home (160,80)", Colors::RoyalBlue, ImVec2( 160, 40 ) );

            ImGuiX::IconButton( EE_ICON_HOME, "Home (160,80) C", Colors::Maroon, ImVec2( 160, 40 ), true );

            ImGuiX::IconButton( EE_ICON_HOME, "##01", Colors::PaleGoldenRod, ImVec2( 160, 40 ) );

            ImGui::SameLine();

            ImGuiX::IconButton( EE_ICON_HOME, "##02", Colors::LightPink, ImVec2( 160, 40 ), true );

            ImGuiX::IconButton( EE_ICON_MOVIE_PLAY, "Play (280,80) Center", Colors::LightPink, ImVec2( 280, 80 ), true );

            ImGuiX::IconButtonColored( EE_ICON_KANGAROO, "Test", Colors::Green, Colors::White, Colors::Yellow, ImVec2( 100, 0 ) );

            ImGuiX::FlatIconButton( EE_ICON_HOME, "Home", Colors::RoyalBlue, ImVec2( 100, 0 ) );
        }

    }

    void UITest::DrawNumericEditors()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Numeric Editors/Helpers" ) )
        {
            static Float2 f2;
            static Float3 f3;
            static Float4 f4;
            static Transform t;

            ImGui::SeparatorText( "Basic Display" );

            ImGuiX::DrawFloat2( f2 );
            ImGuiX::DrawFloat2( "F2", f2, 200);

            ImGuiX::DrawFloat3( f3 );
            ImGuiX::DrawFloat3( "F3", f3, 300 );

            ImGuiX::DrawFloat4( f4 );
            ImGuiX::DrawFloat4( "F4", f4, 240 );

            ImGuiX::DrawTransform( t );
            ImGuiX::DrawTransformNoScale( "NoScale", t, 300 );

            ImGui::SeparatorText( "Basic Input" );

            ImGuiX::InputFloat2( "Float2", f2 );
            ImGuiX::InputFloat3( "Float3", f3 );
            ImGuiX::InputFloat4( "Float4", f4 );

            ImGui::SeparatorText( "Transform" );

            ImGuiX::InputTransform( t );
            ImGuiX::InputTransformNoScale( "TransformNoScale", t );
        }
    }

    void UITest::DrawSpinnersAndAnimated()
    {
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Animated" ) )
        {
            ImGuiX::DrawFlashingText( EE_ICON_ALERT, Colors::Red );
            ImGui::SameLine();
            ImGuiX::DrawFlashingText( EE_ICON_ALERT_CIRCLE_OUTLINE, Colors::Yellow, 2.5f );
            ImGui::SameLine();
            ImGuiX::DrawFlashingText( EE_ICON_ALERT_CIRCLE_OUTLINE, Colors::Pink, 0.5f );
        }

        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Spinners" ) )
        {
            ImGui::Text( "No Size" );
            ImGuiX::DrawSpinner( "S1" );

            ImGuiX::DrawSpinner( "S1-1" );
            ImGui::SameLine();
            ImGui::Button( "ButtonTest" );
            ImGuiX::FlatButton( "Sds" );

            ImGui::Text( "Specified Size" );
            ImGuiX::DrawSpinner( "S2", Colors::Yellow, 100, 10 );

            ImGui::Text( "Fill Remaining Space" );
            ImGuiX::DrawSpinner( "S3", Colors::Blue, -1, 5 );
        }
    }

    void UITest::DrawColorHelpers()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Color Helpers" ) )
        {
            static float f = 0.5f;
            ImGui::SliderFloat( "Gradient Weight", &f, 0.0f, 1.0f );
            {

                ImGui::SeparatorText( "Distinct Color For Zero" );
                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                    ImGui::TextColored( Color::EvaluateRedGreenGradient( f ).ToFloat4(), "Red/Green Gradient" );
                    ImGui::TextColored( Color::EvaluateBlueRedGradient( f ).ToFloat4(), "Blue/Red Gradient" );
                    ImGui::TextColored( Color::EvaluateYellowRedGradient( f ).ToFloat4(), "Yellow/Red Gradient" );
                }

                ImGui::SeparatorText( "No Distinct Color For Zero" );
                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                    ImGui::TextColored( Color::EvaluateRedGreenGradient( f, false ).ToFloat4(), "Red/Green Gradient" );
                    ImGui::TextColored( Color::EvaluateBlueRedGradient( f, false ).ToFloat4(), "Blue/Red Gradient" );
                    ImGui::TextColored( Color::EvaluateYellowRedGradient( f, false ).ToFloat4(), "Yellow/Red Gradient" );
                }
            }
        }
    }

    void UITest::DrawColorCategories()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Category Colors" ) )
        {
            static TVector<StringID> const IDs = 
            { 
                StringID( "Apple" ),
                StringID( "Banana" ),
                StringID( "Cherry" ),
                StringID( "Kiwi" ),
                StringID( "Mango" ),
                StringID( "Orange" ),
                StringID( "Pear" ),
                StringID( "Pineapple" ),
                StringID( "Strawberry" ),
                StringID( "Watermelon" )
            };

            ImGui::SeparatorText( "Category Color" );

            for ( size_t i = 0; i < IDs.size(); i++ )
            {
                ImGui::TextColored( Color::GetCategorizedColor( (int32_t) i ), IDs[i].c_str() );
            }

            ImGui::SeparatorText( "Generated Colors" );

            TVector<Color> colors;
            Color::GenerateColors( (int32_t) IDs.size(), colors );

            for ( size_t i = 0; i < IDs.size(); i++ )
            {
                ImGui::TextColored( colors[i], IDs[i].c_str() );
            }
        }
    }

    void UITest::DrawResourceColors()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Resource Colors" ) )
        {
            THashMap<ResourceTypeID, TypeSystem::ResourceInfo*> const& resourceTypes = m_pContext->m_pTypeRegistry->GetRegisteredResourceTypes();
            for ( auto const& resourceTypePair : resourceTypes )
            {
                TypeSystem::ResourceInfo const* pResourceInfo = resourceTypePair.second;
                ImGui::ColorButton( pResourceInfo->m_friendlyName.c_str(), pResourceInfo->m_color.ToFloat4() );
                ImGui::SameLine();
                ImGui::Text( pResourceInfo->m_friendlyName.c_str() );
            }
        }
    }

    void UITest::DrawHeadersAndSeparators()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Headers/Separators" ) )
        {
            ImGuiX::DrawHeader( "Item Name" );
            ImGuiX::DrawHeader( "Item Name", Colors::Pink, 50 );
            ImGuiX::DrawHeader( EE_ICON_HOME, "Item Name", Colors::Pink );
            ImGuiX::DrawHeader( EE_ICON_CABLE_DATA, "Item Name", Colors::Pink, Colors::White, 50 );
        }
    }

    void UITest::DrawLayoutWidgets()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Layout Widgets" ) )
        {
            ImGuiX::CollapsibleGroupBox( "dasdas", [] () { ImGui::Text( "Contents" ); } );

            ImGuiX::CollapsibleGroupBoxSettings settings;
            settings.m_backgroundColor = ImGuiX::Style::s_colorGray8;
            settings.m_hasBorder = false;
            settings.m_contentsHeight = 200;
            settings.m_rounding = 0;

            ImGuiX::CollapsibleGroupBox( "Seconda", [] ()
            {
                {
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Button( "SADa" );
                }
            }, settings );

            settings.Reset();
            settings.m_headerColor = Colors::LightBlue;
            settings.m_headerTextColor = Colors::Black;
            settings.m_backgroundColor = Colors::CornflowerBlue;
            settings.m_hasBorder = false;
            settings.m_contentsHeight = -1;
            ImGuiX::CollapsibleGroupBox( "Third", [] ()
            {
                {
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Button( "SADa" );
                }
            }, settings );


            settings.Reset();
            settings.m_headerColor = Colors::GoldenRod;
            settings.m_headerTextColor = Colors::Black;
            settings.m_backgroundColor = Colors::PeachPuff;
            settings.m_hasBorder = true;
            settings.m_rounding = 0.0f;
            settings.m_contentsHeight = -1;
            ImGuiX::CollapsibleGroupBox( "Fourth", [] ()
            {
                {
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Text( "Contents" );
                    ImGui::Button( "SADa" );
                }
            }, settings );
        }
    }

    void UITest::DrawSpecialWidgets()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Severity Icons" ) )
        {
            ImGuiX::DrawSeverityIcon( Severity::Info );
            ImGui::SameLine();
            ImGuiX::DrawSeverityIcon( Severity::Warning );
            ImGui::SameLine();
            ImGuiX::DrawSeverityIcon( Severity::Error );
            ImGui::SameLine();
            ImGuiX::DrawSeverityIcon( Severity::FatalError );
        }
    }

    void UITest::DrawMessageBoxTests()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Message Boxes" ) )
        {
            if ( ImGuiX::IconButton( EE_ICON_INFORMATION, "Info Message", Colors::CornflowerBlue ) )
            {
                MessageDialog::Info( "Info", "Some test text" );
            }

            if ( ImGuiX::IconButton( EE_ICON_ALERT, "Warning Message", Colors::Gold ) )
            {
                MessageDialog::Warning( "Warning", "Some test text" );
            }

            if ( ImGuiX::IconButton( EE_ICON_ALERT_OCTAGON, "Error Message", Colors::Red ) )
            {
                MessageDialog::Error( "Error", "Some test text" );
            }
        }
    }

    void UITest::DrawDialogTests()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "File Dialogs" ) )
        {
            if ( ImGui::Button( EE_ICON_FOLDER" Pick Folder" ) )
            {
                FileDialog::Result result = FileDialog::SelectFolder( m_pContext->GetSourceDataDirectory() );
                if ( result )
                {
                    MessageDialog::Info( "Info", "Folder Selected: %s", result.m_filePaths[0].c_str() );
                }
            }

            if ( ImGui::Button( EE_ICON_FILE" Pick File To Load" ) )
            {
                FileDialog::Result result = FileDialog::Load();
                if ( result )
                {
                    MessageDialog::Info( "Info", "File Selected: %s", result.m_filePaths[0].c_str() );
                }
            }

            if ( ImGui::Button( EE_ICON_FILE_STAR" Pick (FBX) File To Load" ) )
            {
                FileDialog::Result result = FileDialog::Load( { FileDialog::ExtensionFilter( "fbx", "Fbx Files" ) } );
                if ( result )
                {
                    MessageDialog::Info( "Info", "File Selected: %s", result.m_filePaths[0].c_str() );
                }
            }

            if ( ImGui::Button( EE_ICON_FILE_MULTIPLE" Pick (MULTIPLE) Files To Load" ) )
            {
                FileDialog::Result result = FileDialog::Load( {}, "Load Multiple Files", true );
                if ( result )
                {
                    InlineString str;
                    for ( auto const& path : result.m_filePaths )
                    {
                        str.append_sprintf( "File: %s\n", path.c_str() );
                    }

                    MessageDialog::Info( "Info", "Files Selected:\n%s", str.c_str() );
                }
            }

            if ( ImGui::Button( EE_ICON_FLOPPY" Pick File To Save" ) )
            {
                FileDialog::Result result = FileDialog::Save();
                if ( result )
                {
                    MessageDialog::Info( "Info", "File Selected: %s", result.m_filePaths[0].c_str() );
                }
            }

            if ( ImGui::Button( EE_ICON_FLOPPY" Pick File To Save (Specified Path)" ) )
            {
                FileDialog::Result result = FileDialog::Save( {}, String(), "D:\\Esoterica\\Data\\File.fbx" );
                if ( result )
                {
                    MessageDialog::Info( "Info", "File Selected: %s", result.m_filePaths[0].c_str() );
                }
            }

            if ( ImGui::Button( EE_ICON_FLOPPY_VARIANT" Pick File To Save" ) )
            {
                FileDialog::Result result = FileDialog::Save( { FileDialog::ExtensionFilter( "fbx", "Fbx Files" ) }, "Save FBX file..." );
                if ( result )
                {
                    MessageDialog::Info( "Info", "File Selected: %s", result.m_filePaths[0].c_str() );
                }
            }
        }
    }

    void UITest::DrawResourcePickers()
    {
        m_picker.UpdateAndDraw();

        m_compactPicker.UpdateAndDraw();
    }

    //-------------------------------------------------------------------------

    void UITest::DrawWindow( bool* pIsWindowOpen )
    {
        if ( ImGui::Begin( "UI Test", pIsWindowOpen, ImGuiWindowFlags_HorizontalScrollbar ) )
        {
            if ( ImGui::BeginTabBar( "Tests" ) )
            {
                if ( ImGui::BeginTabItem( "Basics" ) )
                {
                    DrawFonts();
                    DrawSeparators();
                    DrawTooltips();
                    DrawSpecialWidgets();
                    DrawSpinnersAndAnimated();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Layout" ) )
                {
                    DrawHeadersAndSeparators();
                    DrawLayoutWidgets();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Buttons" ) )
                {
                    DrawIconsInButtons();
                    DrawButtonsWithCalculatedWidth();
                    DrawColoredButtons();
                    DrawIconButtons();
                    DrawDropDownButtons();
                    DrawComboButtons();
                    DrawToggleButtons();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Inputs" ) )
                {
                    DrawInputText();
                    DrawInputCombo();
                    DrawNumericEditors();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Dialogs" ) )
                {
                    DrawMessageBoxTests();
                    DrawDialogTests();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Colors" ) )
                {
                    DrawColorHelpers();

                    DrawColorCategories();

                    DrawResourceColors();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Advanced" ) )
                {
                    DrawResourcePickers();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}