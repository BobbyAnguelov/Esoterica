#include "DebugView_Input.h"
#include "System/Imgui/ImguiX.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Input/InputSystem.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Input
{
    namespace
    {
        static uint32_t const g_controlOutlineColor = 0xFF666666;
        static uint32_t const g_controlFillColor = 0xFF888888;

        static float const g_buttonWidth = 20;
        static float const g_buttonBorderThickness = 2.0f;
        static float const g_buttonBorderRounding = 4.0f;

        static float const g_analogStickRangeRadius = 50;
        static float const g_analogStickPositionRadius = 2;

        static Float2 const g_buttonBorderOffset( g_buttonBorderThickness );
        static Float2 const g_buttonDimensions( g_buttonWidth, g_buttonWidth );
    }

    static void DrawButton( ImDrawList* pDrawList, Float2 const& position, Float2 const& dimensions, char const* const pLabel, bool IsHeldDown, uint32_t buttonColor = g_controlOutlineColor, uint32_t pressedColor = g_controlFillColor )
    {
        EE_ASSERT( pDrawList != nullptr );
        Float2 const buttonTopLeft = position;
        Float2 const buttonBottomRight = buttonTopLeft + dimensions;
        pDrawList->AddRect( buttonTopLeft, buttonBottomRight, buttonColor, g_buttonBorderRounding, ImDrawFlags_RoundCornersAll, g_buttonBorderThickness );

        if ( IsHeldDown )
        {
            pDrawList->AddRectFilled( buttonTopLeft + g_buttonBorderOffset, buttonTopLeft + dimensions - g_buttonBorderOffset, pressedColor, g_buttonBorderRounding - 1, ImDrawFlags_RoundCornersAll );
        }

        if ( pLabel != nullptr )
        {
            Float2 const textDimensions = ImGui::CalcTextSize( pLabel );
            Float2 const textPos = Float2( buttonTopLeft.m_x + ( dimensions.m_x / 2 ) - ( textDimensions.m_x / 2 ) + 1, buttonTopLeft.m_y + ( dimensions.m_y / 2 ) - ( textDimensions.m_y / 2 ) );
            pDrawList->AddText( textPos, 0xFFFFFFFF, pLabel );
        }
    }

    static void DrawTriggerButton( ImDrawList* pDrawList, Float2 const& position, Float2 const& dimensions, char const* const pLabel, ControllerInputState const& controllerState, bool isLeftTrigger )
    {
        EE_ASSERT( pDrawList != nullptr );

        // Draw the label if set
        Float2 drawPosition = position;
        if ( pLabel != nullptr )
        {
            Float2 const textDimensions = ImGui::CalcTextSize( pLabel );
            Float2 const textPos = Float2( drawPosition.m_x + ( dimensions.m_x / 2 ) - ( textDimensions.m_x / 2 ) + 1, drawPosition.m_y + g_buttonBorderThickness );
            pDrawList->AddText( textPos, 0xFFFFFFFF, pLabel );
            drawPosition.m_y += textDimensions.m_y + 4;
        }

        // Draw the border
        Float2 const borderDimensions( dimensions.m_x, dimensions.m_y - ( drawPosition.m_y - position.m_y + 4 ) );
        Float2 const triggerTopLeft = drawPosition;
        Float2 const triggerBottomRight = triggerTopLeft + borderDimensions;
        pDrawList->AddRect( triggerTopLeft, triggerBottomRight, g_controlOutlineColor, 0.0f, ImDrawFlags_RoundCornersAll, g_buttonBorderThickness );

        // Draw the trigger values
        float const triggerValueRaw = isLeftTrigger ? controllerState.GetLeftTriggerRawValue() : controllerState.GetRightTriggerRawValue();
        if ( triggerValueRaw > 0 )
        {
            float triggerValue0;
            uint32_t triggerValue0Color;

            float triggerValue1;
            uint32_t triggerValue1Color;

            if ( isLeftTrigger )
            {
                triggerValue0 = controllerState.GetLeftTriggerRawValue();
                triggerValue1 = controllerState.GetLeftTriggerValue();
                triggerValue0Color = 0xFF0000FF;
                triggerValue1Color = 0xFF00FF00;
            }
            else
            {
                triggerValue0 = controllerState.GetRightTriggerValue();
                triggerValue1 = controllerState.GetRightTriggerRawValue();
                triggerValue0Color = 0xFF00FF00;
                triggerValue1Color = 0xFF0000FF;
            }

            float const valueMaxLength = borderDimensions.m_y - ( g_buttonBorderThickness * 2 );
            float const triggerValueWidth = ( borderDimensions.m_x - g_buttonBorderThickness * 2 ) / 2;
            float const triggerValue0TopLeftX = drawPosition.m_x + g_buttonBorderThickness;
            float const triggerValue1TopLeftX = triggerValue0TopLeftX + triggerValueWidth;
            float const triggerValue0TopLeftY = drawPosition.m_y + g_buttonBorderThickness + ( 1.0f - triggerValue0 ) * valueMaxLength;
            float const triggerValue1TopLeftY = drawPosition.m_y + g_buttonBorderThickness + ( 1.0f - triggerValue1 ) * valueMaxLength;

            Float2 const triggerValue0TopLeft( triggerValue0TopLeftX, triggerValue0TopLeftY );
            Float2 const triggerValue0BottomRight( triggerValue1TopLeftX, triggerBottomRight.m_y - g_buttonBorderThickness );
            pDrawList->AddRectFilled( triggerValue0TopLeft, triggerValue0BottomRight, triggerValue0Color );

            Float2 const triggerValue1TopLeft( triggerValue0TopLeftX + triggerValueWidth, triggerValue1TopLeftY );
            Float2 const triggerValue1BottomRight( triggerValue1TopLeftX + triggerValueWidth, triggerBottomRight.m_y - g_buttonBorderThickness );
            pDrawList->AddRectFilled( triggerValue1TopLeft, triggerValue1BottomRight, triggerValue1Color );
        }
    }

    static void DrawAnalogStick( ImDrawList* pDrawList, Float2 const position, ControllerInputDevice const& controller, bool isLeftStick )
    {
        EE_ASSERT( pDrawList != nullptr );

        auto const& settings = controller.GetSettings();
        auto const& controllerState = controller.GetControllerState();

        Float2 rawValue = isLeftStick ? controllerState.GetLeftAnalogStickRawValue() : controllerState.GetRightAnalogStickRawValue();
        Float2 filteredValue = isLeftStick ? controllerState.GetLeftAnalogStickValue() : controllerState.GetRightAnalogStickValue();

        // Invert the m_y values to match screen space
        rawValue.m_y = -rawValue.m_y;
        filteredValue.m_y = -filteredValue.m_y;

        // Draw max stick range and dead zone range
        float const innerDeadZoneRadius = g_analogStickRangeRadius * ( isLeftStick ? settings.m_leftStickInnerDeadzone : settings.m_rightStickInnerDeadzone );
        float const outerDeadZoneRadius = g_analogStickRangeRadius * ( 1.0f - ( isLeftStick ? settings.m_leftStickOuterDeadzone : settings.m_rightStickOuterDeadzone ) );
        Float2 const analogStickCenter = position + Float2( g_analogStickRangeRadius );
        pDrawList->AddCircle( analogStickCenter, g_analogStickRangeRadius, g_controlFillColor, 20 );
        pDrawList->AddCircleFilled( analogStickCenter, outerDeadZoneRadius, g_controlFillColor, 20 );
        pDrawList->AddCircleFilled( analogStickCenter, innerDeadZoneRadius, 0xFF333333, 20 );

        // Draw raw stick position
        Float2 stickOffset = rawValue * g_analogStickRangeRadius;
        pDrawList->AddCircleFilled( analogStickCenter + stickOffset, g_analogStickPositionRadius, 0xFF0000FF, 6 );

        // Draw filtered stick position
        Vector vDirection = Vector( filteredValue ).GetNormalized2();
        stickOffset = ( filteredValue * ( outerDeadZoneRadius - innerDeadZoneRadius ) ) + ( vDirection * innerDeadZoneRadius ).ToFloat2();
        pDrawList->AddCircleFilled( analogStickCenter + stickOffset, g_analogStickPositionRadius, 0xFF00FF00, 6 );
    }

    //-------------------------------------------------------------------------

    void InputDebugView::DrawControllerState( ControllerInputDevice const& controller )
    {
        auto const& controllerState = controller.GetControllerState();

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        Float2 const FirstRowTopLeft = ImGui::GetCursorScreenPos();
        Float2 const triggerButtonDimensions( g_buttonWidth, ( g_analogStickRangeRadius * 2 ) + ( g_buttonWidth * 2 ) + 8 );

        Float2 totalSize;
        Float2 drawPosition;

        // Left Shoulder and trigger buttons
        drawPosition = FirstRowTopLeft;
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "LB", controllerState.IsHeldDown( Input::ControllerButton::ShoulderLeft ) );
        drawPosition.m_y += g_buttonDimensions.m_y;
        DrawTriggerButton( pDrawList, drawPosition, triggerButtonDimensions, "LT", controllerState, true );

        // Left analog stick
        drawPosition = Float2( drawPosition.m_x + g_buttonDimensions.m_x + 9, FirstRowTopLeft.m_y );
        DrawAnalogStick( pDrawList, drawPosition, controller, true );

        // Right analog stick
        drawPosition = Float2( drawPosition.m_x + 26 + g_analogStickRangeRadius * 2, FirstRowTopLeft.m_y );
        DrawAnalogStick( pDrawList, drawPosition, controller, false );

        // Right Shoulder and trigger buttons
        drawPosition = Float2( drawPosition.m_x + g_analogStickRangeRadius * 2 + 9, FirstRowTopLeft.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "RB", controllerState.IsHeldDown( Input::ControllerButton::ShoulderRight ) );
        drawPosition.m_y += g_buttonDimensions.m_y;
        DrawTriggerButton( pDrawList, drawPosition, triggerButtonDimensions, "RT", controllerState, false );

        totalSize.m_x = ( drawPosition.m_x + g_buttonWidth ) - FirstRowTopLeft.m_x;
        totalSize.m_y = ( g_analogStickRangeRadius * 2 ) + 8;

        //-------------------------------------------------------------------------

        Float2 const SecondRowTopLeft = Float2( FirstRowTopLeft.m_x, FirstRowTopLeft.m_y + totalSize.m_y );

        // D-Pad
        float const upButtonTopLeft = SecondRowTopLeft.m_x + ( g_buttonWidth + 9 + g_analogStickRangeRadius ) - g_buttonWidth / 2;

        drawPosition = Float2( upButtonTopLeft, SecondRowTopLeft.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Up", controllerState.IsHeldDown( Input::ControllerButton::DPadUp ) );
        drawPosition = Float2( upButtonTopLeft - g_buttonWidth, SecondRowTopLeft.m_y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Lt", controllerState.IsHeldDown( Input::ControllerButton::DPadLeft ) );
        drawPosition = Float2( upButtonTopLeft + g_buttonWidth, SecondRowTopLeft.m_y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Rt", controllerState.IsHeldDown( Input::ControllerButton::DPadRight ) );
        drawPosition = Float2( upButtonTopLeft, SecondRowTopLeft.m_y + g_buttonWidth + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Dn", controllerState.IsHeldDown( Input::ControllerButton::DPadDown ) );

        // Face Buttons
        float const topFaceButtonTopLeft = SecondRowTopLeft.m_x + ( ( g_buttonWidth + g_analogStickRangeRadius ) - g_buttonWidth / 2 ) * 2 + 34 + ( g_buttonWidth * 2 );

        drawPosition = Float2( topFaceButtonTopLeft, SecondRowTopLeft.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Y", controllerState.IsHeldDown( Input::ControllerButton::FaceButtonUp ), 0xFF00FFFF );
        drawPosition = Float2( topFaceButtonTopLeft - g_buttonWidth, SecondRowTopLeft.m_y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "X", controllerState.IsHeldDown( Input::ControllerButton::FaceButtonLeft ), 0xFFFF0000 );
        drawPosition = Float2( topFaceButtonTopLeft + g_buttonWidth, SecondRowTopLeft.m_y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "B", controllerState.IsHeldDown( Input::ControllerButton::FaceButtonRight ), 0xFF0000FF );
        drawPosition = Float2( topFaceButtonTopLeft, SecondRowTopLeft.m_y + g_buttonWidth + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "A", controllerState.IsHeldDown( Input::ControllerButton::FaceButtonDown ), 0xFF00FF00 );

        // System Buttons
        drawPosition = Float2( SecondRowTopLeft.m_x + g_buttonWidth + g_analogStickRangeRadius * 2, SecondRowTopLeft.m_y + 10 );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "S0", controllerState.IsHeldDown( Input::ControllerButton::System0 ) );
        drawPosition = Float2( drawPosition.m_x + g_buttonWidth + 4, drawPosition.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "S1", controllerState.IsHeldDown( Input::ControllerButton::System1 ) );

        // Stick Buttons
        drawPosition = Float2( SecondRowTopLeft.m_x + g_buttonWidth + g_analogStickRangeRadius * 2, drawPosition.m_y + g_buttonWidth + 4 );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "LS", controllerState.IsHeldDown( Input::ControllerButton::ThumbstickLeft ) );
        drawPosition = Float2( drawPosition.m_x + g_buttonWidth + 4, drawPosition.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "RS", controllerState.IsHeldDown( Input::ControllerButton::ThumbstickRight ) );

        totalSize.m_x = ( drawPosition.m_x + g_buttonWidth ) - FirstRowTopLeft.m_x;
        totalSize.m_y = triggerButtonDimensions.m_y + g_buttonWidth + 4;

        //-------------------------------------------------------------------------
        ImGui::Dummy( totalSize );
    }

    //-------------------------------------------------------------------------

    InputDebugView::InputDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Engine/Input", [this] ( EntityWorldUpdateContext const& context ) { DrawControllerMenu( context ); } ) );
    }

    void InputDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        EntityWorldDebugView::Initialize( systemRegistry, pWorld );
        m_pInputSystem = systemRegistry.GetSystem<InputSystem>();
    }

    void InputDebugView::Shutdown()
    {
        m_pInputSystem = nullptr;
        EntityWorldDebugView::Shutdown();
    }

    void InputDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        auto pInputSystem = context.GetSystem<InputSystem>();
        EE_ASSERT( pInputSystem != nullptr );

        char buffer[256];

        // Draw the open controller windows
        int32_t const numControllers = (int32_t) m_openControllerWindows.size();
        for ( int32_t i = numControllers - 1; i >= 0; i-- )
        {
            // Draw the window
            Printf( buffer, 256, "Controller State: Controller %d", i );

            bool isWindowOpen = true;
            ImGui::SetNextWindowSize( ImVec2( 300, 210 ) );
            if ( pWindowClass != nullptr ) ImGui::SetNextWindowClass( pWindowClass );
            if ( ImGui::Begin( buffer, &isWindowOpen, ImGuiWindowFlags_NoResize ) )
            {
                DrawControllerState( *m_openControllerWindows[i] );
            }
            ImGui::End();

            // Should we close the window?
            if ( !isWindowOpen )
            {
                m_openControllerWindows.erase_unsorted( m_openControllerWindows.begin() + i );
            }
        }
    }

    void InputDebugView::DrawControllerMenu( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_pInputSystem != nullptr );

        //-------------------------------------------------------------------------

        ImGui::MenuItem( "Mouse State" );

        ImGui::MenuItem( "Keyboard State" );

        //-------------------------------------------------------------------------

        ImGui::Separator();

        uint32_t const numControllers = m_pInputSystem->GetNumConnectedControllers();
        if ( numControllers > 0 )
        {
            TInlineString<100> str;
            bool noControllersConnected = true;
            for ( uint32_t i = 0u; i < numControllers; i++ )
            {
                str.sprintf( "Show Controller State: %d", i );
                if ( ImGui::MenuItem( str.c_str() ) )
                {
                    auto pController = m_pInputSystem->GetControllerDevice( i );
                    bool isWindowAlreadyOpen = VectorContains( m_openControllerWindows, pController );
                    if ( !isWindowAlreadyOpen )
                    {
                        m_openControllerWindows.emplace_back( pController );
                    }
                }
            }
        }
        else
        {
            ImGui::Text( "No Controllers Connected" );
        }
    }
}
#endif