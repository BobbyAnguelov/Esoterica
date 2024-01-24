#include "DebugView_Input.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Player/Systems/WorldSystem_PlayerManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Input/InputSystem.h"
#include "Engine/Player/Components/Component_Player.h"

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

        static StringID const g_controllerWindowTypeID( "ControllerWindow" );
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

    static void DrawTriggerButton( ImDrawList* pDrawList, Float2 const& position, Float2 const& dimensions, char const* const pLabel, ControllerDevice const& controller, bool isLeftTrigger )
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

        // Get Values
        float triggerValue = 0.0f;
        float triggerRawValue = 0.0f;
        uint32_t triggerValueColor = 0;
        uint32_t triggerRawValueColor = 0;

        if ( isLeftTrigger )
        {
            triggerValue = controller.GetValue( InputID::Controller_LeftTrigger );
            triggerRawValue = controller.GetRawValue( InputID::Controller_LeftTrigger );
            triggerValueColor = 0xFF0000FF;
            triggerRawValueColor = 0xFF00FF00;
        }
        else
        {
            triggerValue = controller.GetValue( InputID::Controller_RightTrigger );
            triggerRawValue = controller.GetRawValue( InputID::Controller_RightTrigger );
            triggerValueColor = 0xFF00FF00;
            triggerRawValueColor = 0xFF0000FF;
        }

        // Draw the trigger values
        if ( triggerRawValue > 0 )
        {
            float const valueMaxLength = borderDimensions.m_y - ( g_buttonBorderThickness * 2 );
            float const triggerValueWidth = ( borderDimensions.m_x - g_buttonBorderThickness * 2 ) / 2;
            float const triggerValue0TopLeftX = drawPosition.m_x + g_buttonBorderThickness;
            float const triggerValue1TopLeftX = triggerValue0TopLeftX + triggerValueWidth;
            float const triggerValue0TopLeftY = drawPosition.m_y + g_buttonBorderThickness + ( 1.0f - triggerValue ) * valueMaxLength;
            float const triggerValue1TopLeftY = drawPosition.m_y + g_buttonBorderThickness + ( 1.0f - triggerRawValue ) * valueMaxLength;

            Float2 const triggerValue0TopLeft( triggerValue0TopLeftX, triggerValue0TopLeftY );
            Float2 const triggerValue0BottomRight( triggerValue1TopLeftX, triggerBottomRight.m_y - g_buttonBorderThickness );
            pDrawList->AddRectFilled( triggerValue0TopLeft, triggerValue0BottomRight, triggerValueColor );

            Float2 const triggerValue1TopLeft( triggerValue0TopLeftX + triggerValueWidth, triggerValue1TopLeftY );
            Float2 const triggerValue1BottomRight( triggerValue1TopLeftX + triggerValueWidth, triggerBottomRight.m_y - g_buttonBorderThickness );
            pDrawList->AddRectFilled( triggerValue1TopLeft, triggerValue1BottomRight, triggerRawValueColor );
        }
    }

    static void DrawAnalogStick( ImDrawList* pDrawList, Float2 const position, ControllerDevice const& controller, bool isLeftStick )
    {
        EE_ASSERT( pDrawList != nullptr );

        Float2 rawValue = Float2::Zero;
        Float2 filteredValue = Float2::Zero;

        if ( isLeftStick )
        {
            rawValue = Float2( controller.GetRawValue( InputID::Controller_LeftStickHorizontal ), controller.GetRawValue( InputID::Controller_LeftStickVertical ) );
            filteredValue = Float2( controller.GetValue( InputID::Controller_LeftStickHorizontal ), controller.GetValue( InputID::Controller_LeftStickVertical ) );
        }
        else
        {
            rawValue = Float2( controller.GetRawValue( InputID::Controller_RightStickHorizontal ), controller.GetRawValue( InputID::Controller_RightStickVertical ) );
            filteredValue = Float2( controller.GetValue( InputID::Controller_RightStickHorizontal ), controller.GetValue( InputID::Controller_RightStickVertical ) );
        }

        // Invert the m_y values to match screen space
        rawValue.m_y = -rawValue.m_y;
        filteredValue.m_y = -filteredValue.m_y;

        // Draw max stick range and dead zone range
        float const innerDeadZoneRadius = g_analogStickRangeRadius * ( isLeftStick ? controller.GetLeftStickInnerDeadzone() : controller.GetRightStickInnerDeadzone() );
        float const outerDeadZoneRadius = g_analogStickRangeRadius * ( 1.0f - ( isLeftStick ? controller.GetLeftStickOuterDeadzone() : controller.GetRightStickOuterDeadzone() ) );
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

    void InputDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pInputSystem = systemRegistry.GetSystem<InputSystem>();
        m_pPlayerManager = pWorld->GetWorldSystem<PlayerManager>();

        //-------------------------------------------------------------------------

        m_windows.emplace_back( "Draw Virtual Inputs", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawVirtualInputState( context ); } );
    }

    void InputDebugView::Shutdown()
    {
        m_pPlayerManager = nullptr;
        m_pInputSystem = nullptr;
        DebugView::Shutdown();
    }

    void InputDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_pInputSystem != nullptr );

        //-------------------------------------------------------------------------

        if ( context.IsGameWorld() && m_pPlayerManager->HasPlayer() )
        {
            ImGui::MenuItem( "Virtual Inputs", nullptr, &m_windows[0].m_isOpen );
        }

        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Device State" );

        //-------------------------------------------------------------------------

        ImGui::MenuItem( "Mouse State (TODO)" );

        ImGui::MenuItem( "Keyboard State (TODO)" );

        //-------------------------------------------------------------------------

        ImGui::Separator();

        if ( m_numControllers > 0 )
        {
            TInlineString<100> str;
            for ( int32_t i = 0; i < m_numControllers; i++ )
            {
                str.sprintf( "Show Controller State: %d", i );
                if ( ImGui::MenuItem( str.c_str() ) )
                {
                    for ( auto& window : m_windows )
                    {
                        if ( m_windows[i].m_typeID == g_controllerWindowTypeID && m_windows[i].m_userData == i )
                        {
                            m_windows[i].m_isOpen = true;
                        }
                    }
                }
            }
        }
        else
        {
            ImGui::Text( "No Controllers Connected" );
        }
    }

    void InputDebugView::Update( EntityWorldUpdateContext const& context )
    {
        // Close the virtual input state if there is no player
        //-------------------------------------------------------------------------

        if ( !m_pPlayerManager->HasPlayer() )
        {
            m_windows[0].m_isOpen = false;
        }

        // Update controller window state
        //-------------------------------------------------------------------------

        int32_t const numControllers = m_pInputSystem->GetNumConnectedControllers();
        if ( numControllers != m_numControllers )
        {
            m_numControllers = numControllers;

            // Remove all controller windows
            for ( int32_t i = (int32_t) m_windows.size() - 1; i >= 0; i-- )
            {
                if ( m_windows[i].m_typeID == g_controllerWindowTypeID )
                {
                    m_windows.erase_unsorted( m_windows.begin() + i );
                }
            }

            // Create controller windows
            TInlineString<100> str;
            for ( int32_t i = 0; i < numControllers; i++ )
            {
                auto DrawControllerStateLambda = [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData )
                {
                    auto pControllerDevice = m_pInputSystem->GetController( (uint32_t) userData );
                    DrawControllerState( context, *pControllerDevice );
                };

                str.sprintf( "Controller State: Controller %d", i );
                m_windows.emplace_back( str.c_str(), DrawControllerStateLambda );
                m_windows.back().m_typeID = g_controllerWindowTypeID;
                m_windows.back().m_userData = i;
            }
        }
    }

    void InputDebugView::DrawVirtualInputState( EntityWorldUpdateContext const& context )
    {
        EntityID const playerID = m_pPlayerManager->GetPlayerEntityID();
        Entity* pPlayerEntity = m_pWorld->FindEntity( playerID );
        EE_ASSERT( pPlayerEntity != nullptr );

        //-------------------------------------------------------------------------

        Player::PlayerComponent const* pPlayerComponent = nullptr;
        for ( auto pComponent : pPlayerEntity->GetComponents() )
        {
            pPlayerComponent = TryCast<Player::PlayerComponent>( pComponent );
            if ( pPlayerComponent != nullptr )
            {
                break;
            }
        }
        
        if ( pPlayerComponent == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        VirtualInputRegistry const* pInputRegistry = pPlayerComponent->GetInputRegistry();

        for ( auto pInput : pInputRegistry->m_inputs )
        {
            ImGui::PushID( pInput );

            //-------------------------------------------------------------------------

            ImGui::Text( pInput->GetID().c_str() );

            ImGui::SameLine();

            switch ( pInput->GetType() )
            {
                case VirtualInput::Type::Binary:
                {
                    auto pActualInput = static_cast<BinaryInput const*>( pInput );
                    ImGui::Text( pActualInput->GetValue() ? "true" : "false" );
                }
                break;

                case VirtualInput::Type::Analog:
                {
                    auto pActualInput = static_cast<AnalogInput const*>( pInput );
                    ImGui::Text( "%.2f", pActualInput->GetValue() );
                }
                break;

                case VirtualInput::Type::Directional:
                {
                    auto pActualInput = static_cast<DirectionalInput const*>( pInput );
                    Float2 value = pActualInput->GetValue();
                    ImGuiX::DrawFloat2( value, -1 );
                }
                break;
            }

            //-------------------------------------------------------------------------

            ImGui::PopID();
        }
    }

    void InputDebugView::DrawControllerState( EntityWorldUpdateContext const& context, ControllerDevice const& controller )
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        Float2 const FirstRowTopLeft = ImGui::GetCursorScreenPos();
        Float2 const triggerButtonDimensions( g_buttonWidth, ( g_analogStickRangeRadius * 2 ) + ( g_buttonWidth * 2 ) + 8 );

        Float2 totalSize;
        Float2 drawPosition;

        // Left Shoulder and trigger buttons
        drawPosition = FirstRowTopLeft;
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "LB", controller.IsHeldDown( Input::InputID::Controller_ShoulderLeft ) );
        drawPosition.m_y += g_buttonDimensions.m_y;
        DrawTriggerButton( pDrawList, drawPosition, triggerButtonDimensions, "LT", controller, true );

        // Left analog stick
        drawPosition = Float2( drawPosition.m_x + g_buttonDimensions.m_x + 9, FirstRowTopLeft.m_y );
        DrawAnalogStick( pDrawList, drawPosition, controller, true );

        // Right analog stick
        drawPosition = Float2( drawPosition.m_x + 26 + g_analogStickRangeRadius * 2, FirstRowTopLeft.m_y );
        DrawAnalogStick( pDrawList, drawPosition, controller, false );

        // Right Shoulder and trigger buttons
        drawPosition = Float2( drawPosition.m_x + g_analogStickRangeRadius * 2 + 9, FirstRowTopLeft.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "RB", controller.IsHeldDown( Input::InputID::Controller_ShoulderRight ) );
        drawPosition.m_y += g_buttonDimensions.m_y;
        DrawTriggerButton( pDrawList, drawPosition, triggerButtonDimensions, "RT", controller, false );

        totalSize.m_x = ( drawPosition.m_x + g_buttonWidth ) - FirstRowTopLeft.m_x;
        totalSize.m_y = ( g_analogStickRangeRadius * 2 ) + 8;

        //-------------------------------------------------------------------------

        Float2 const SecondRowTopLeft = Float2( FirstRowTopLeft.m_x, FirstRowTopLeft.m_y + totalSize.m_y );

        // D-Pad
        float const upButtonTopLeft = SecondRowTopLeft.m_x + ( g_buttonWidth + 9 + g_analogStickRangeRadius ) - g_buttonWidth / 2;

        drawPosition = Float2( upButtonTopLeft, SecondRowTopLeft.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Up", controller.IsHeldDown( Input::InputID::Controller_DPadUp ) );
        drawPosition = Float2( upButtonTopLeft - g_buttonWidth, SecondRowTopLeft.m_y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Lt", controller.IsHeldDown( Input::InputID::Controller_DPadLeft ) );
        drawPosition = Float2( upButtonTopLeft + g_buttonWidth, SecondRowTopLeft.m_y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Rt", controller.IsHeldDown( Input::InputID::Controller_DPadRight ) );
        drawPosition = Float2( upButtonTopLeft, SecondRowTopLeft.m_y + g_buttonWidth + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Dn", controller.IsHeldDown( Input::InputID::Controller_DPadDown ) );

        // Face Buttons
        float const topFaceButtonTopLeft = SecondRowTopLeft.m_x + ( ( g_buttonWidth + g_analogStickRangeRadius ) - g_buttonWidth / 2 ) * 2 + 34 + ( g_buttonWidth * 2 );

        drawPosition = Float2( topFaceButtonTopLeft, SecondRowTopLeft.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "Y", controller.IsHeldDown( Input::InputID::Controller_FaceButtonUp ), 0xFF00FFFF );
        drawPosition = Float2( topFaceButtonTopLeft - g_buttonWidth, SecondRowTopLeft.m_y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "X", controller.IsHeldDown( Input::InputID::Controller_FaceButtonLeft ), 0xFFFF0000 );
        drawPosition = Float2( topFaceButtonTopLeft + g_buttonWidth, SecondRowTopLeft.m_y + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "B", controller.IsHeldDown( Input::InputID::Controller_FaceButtonRight ), 0xFF0000FF );
        drawPosition = Float2( topFaceButtonTopLeft, SecondRowTopLeft.m_y + g_buttonWidth + g_buttonWidth );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "A", controller.IsHeldDown( Input::InputID::Controller_FaceButtonDown ), 0xFF00FF00 );

        // System Buttons
        drawPosition = Float2( SecondRowTopLeft.m_x + g_buttonWidth + g_analogStickRangeRadius * 2, SecondRowTopLeft.m_y + 10 );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "S0", controller.IsHeldDown( Input::InputID::Controller_System0 ) );
        drawPosition = Float2( drawPosition.m_x + g_buttonWidth + 4, drawPosition.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "S1", controller.IsHeldDown( Input::InputID::Controller_System1 ) );

        // Stick Buttons
        drawPosition = Float2( SecondRowTopLeft.m_x + g_buttonWidth + g_analogStickRangeRadius * 2, drawPosition.m_y + g_buttonWidth + 4 );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "LS", controller.IsHeldDown( Input::InputID::Controller_ThumbstickLeft ) );
        drawPosition = Float2( drawPosition.m_x + g_buttonWidth + 4, drawPosition.m_y );
        DrawButton( pDrawList, drawPosition, g_buttonDimensions, "RS", controller.IsHeldDown( Input::InputID::Controller_ThumbstickRight ) );

        totalSize.m_x = ( drawPosition.m_x + g_buttonWidth ) - FirstRowTopLeft.m_x;
        totalSize.m_y = triggerButtonDimensions.m_y + g_buttonWidth + 4;

        //-------------------------------------------------------------------------
        ImGui::Dummy( totalSize );
    }
}
#endif