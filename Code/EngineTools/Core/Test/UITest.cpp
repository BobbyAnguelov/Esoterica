#include "UITest.h"
#include "Base/Imgui/ImguiX.h"
#include "EngineTools/Core/Dialogs.h"
#include "EngineTools/Core/ToolsContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    static void DrawFonts()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Fonts" ) )
        {
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Tiny );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Tiny" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::TinyBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Tiny Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Small" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Small Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Medium" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                ImGui::Text( EE_ICON_ALERT"This is a test - Medium Bold" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                ImGui::Text( EE_ICON_FILE_CHECK"This is a test - Large" );
            }
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                ImGui::Text( EE_ICON_CCTV_OFF"This is a test - Large Bold" );
            }
        }
    }

    static void DrawIconsInButtons()
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

    static void DrawSeparators()
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

    static void DrawColoredButtons()
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

    static void DrawTooltips()
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

    static void DrawDropDownButtons()
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

    static void DrawComboButtons()
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

    static void DrawInputText()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Input Text" ) )
        {
            auto TestCombo = []
            {
                ImGui::Text( "Test" );
            };

            static char buffer[255];

            ImGuiX::InputTextCombo( "ITC0", buffer, 255, TestCombo );

            ImGui::SetNextItemWidth( 200 );
            ImGuiX::InputTextCombo( "ITC1", buffer, 255, TestCombo );
            ImGui::SameLine();
            ImGui::SetNextItemWidth( 200 );
            ImGuiX::InputTextCombo( "ITC2", buffer, 255, TestCombo );

            ImGui::SetNextItemWidth( -1 );
            ImGuiX::InputTextCombo( "ITC3", buffer, 255, TestCombo );

            //-------------------------------------------------------------------------

            ImGuiX::InputTextWithClearButton( "ITCB0", "width: 0", buffer, 255 );

            ImGui::SetNextItemWidth( 200 );
            ImGuiX::InputTextWithClearButton( "ITCB1", "width: 200", buffer, 255 );

            ImGui::SetNextItemWidth( -1 );
            ImGuiX::InputTextWithClearButton( "ITCB2", "width: -1 ", buffer, 255 );

            //-------------------------------------------------------------------------

            ImGuiX::InputTextComboWithClearButton( "ITCCB0", "width: 0", buffer, 255, TestCombo );

            ImGui::SetNextItemWidth( 200 );
            ImGuiX::InputTextComboWithClearButton( "ITCCB1", "width: 200", buffer, 255, TestCombo );

            ImGui::SetNextItemWidth( -1 );
            ImGuiX::InputTextComboWithClearButton( "ITCCB2", "width: -1 ", buffer, 255, TestCombo );
        }
    }

    static void DrawIconButtons()
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

            ImGuiX::IconButton( EE_ICON_HOME, "", Colors::PaleGoldenRod, ImVec2( 160, 40 ) );

            ImGui::SameLine();

            ImGuiX::IconButton( EE_ICON_HOME, "", Colors::LightPink, ImVec2( 160, 40 ), true );

            ImGuiX::IconButton( EE_ICON_MOVIE_PLAY, "Play (280,80) Center", Colors::LightPink, ImVec2( 280, 80 ), true );

            ImGuiX::IconButtonColored( EE_ICON_KANGAROO, "Test", Colors::Green, Colors::White, Colors::Yellow, ImVec2( 100, 0 ) );

            ImGuiX::FlatIconButton( EE_ICON_HOME, "Home", Colors::RoyalBlue, ImVec2( 100, 0 ) );
        }

    }

    static void DrawNumericEditors()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Numeric Editors/Helpers" ) )
        {
            static Float2 f2;
            static Float3 f3;
            static Float4 f4;
            static Transform t;

            ImGuiX::DrawFloat2( f2 );
            ImGuiX::DrawFloat3( f3 );
            ImGuiX::DrawFloat4( f4 );
            ImGuiX::DrawTransform( t );

            ImGui::Separator();

            ImGuiX::InputFloat2( "Float2", f2 );
            ImGuiX::InputFloat3( "Float3", f3 );
            ImGuiX::InputFloat4( "Float4", f4 );
            ImGuiX::InputTransform( "Float4", t );

            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "prefix" );
            ImGui::SameLine();
            ImGuiX::InputFloat2( "blah", f2 );
            ImGui::SameLine();
            ImGui::Text( "suffix" );
        }
    }

    static void DrawSpinners()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Spinners" ) )
        {
            ImGui::Text( "No Size" );
            ImGuiX::DrawSpinner( "S1" );

            ImGui::Text( "Specified Size" );
            ImGuiX::DrawSpinner( "S2", Colors::Yellow, 100, 10 );

            ImGui::Text( "Fill Remaining Space" );
            ImGuiX::DrawSpinner( "S3", Colors::Blue, -1, 5 );
        }
    }

    static void DrawColorHelpers()
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Color Helpers" ) )
        {
            static float f = 0.0f;
            ImGui::SliderFloat( "Gradient Weight", &f, 0.0f, 1.0f );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                ImGui::TextColored( Color::EvaluateRedGreenGradient( f ).ToFloat4(), "Red/Green Gradient" );
                ImGui::TextColored( Color::EvaluateBlueRedGradient( f ).ToFloat4(), "Blue/Red Gradient" );
                ImGui::TextColored( Color::EvaluateYellowRedGradient( f ).ToFloat4(), "Yellow/Red Gradient" );
            }
        }
    }

    static void DrawMessageBoxTests()
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

    static void DrawDialogTests( ToolsContext* pContext )
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "File Dialogs" ) )
        {
            if ( ImGui::Button( EE_ICON_FOLDER" Pick Folder" ) )
            {
                FileDialog::Result result = FileDialog::SelectFolder( pContext->GetSourceDataDirectory() );
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

    //-------------------------------------------------------------------------

    void DrawUITestWindow( ToolsContext* pContext, bool* pIsWindowOpen )
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
                    DrawSpinners();
                    DrawColorHelpers();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Buttons" ) )
                {
                    DrawIconsInButtons();
                    DrawColoredButtons();
                    DrawIconButtons();
                    DrawDropDownButtons();
                    DrawComboButtons();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Inputs" ) )
                {
                    DrawInputText();
                    DrawNumericEditors();

                    ImGui::EndTabItem();
                }

                if ( ImGui::BeginTabItem( "Dialogs" ) )
                {
                    DrawMessageBoxTests();
                    DrawDialogTests( pContext );

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}