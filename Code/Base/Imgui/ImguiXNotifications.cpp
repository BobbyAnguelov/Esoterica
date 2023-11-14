#include "ImguiXNotifications.h"
#include "MaterialDesignIcons.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    class EE_BASE_API Notification
    {
        constexpr static float const s_defaultLifetime = 3000;
        constexpr static float const s_defaultFadeTime = 150;

    public:

        enum class Type
        {
            None,
            Success,
            Warning,
            Error,
            Info,
        };

        enum class Phase
        {
            FadeIn,
            Wait,
            FadeOut,
            Expired,
        };

        enum class Position
        {
            TopLeft,
            TopCenter,
            TopRight,
            BottomLeft,
            BottomCenter,
            BottomRight,
            Center,
        };

    public:

        Notification( Type type, InlineString&& message )
            : m_type( type )
            , m_message( message )
        {
            EE_ASSERT( type != Type::None );
            m_timer.Start();
        }

        Type GetType() const { return m_type; };

        Phase GetPhase() const
        {
            float const elapsedTime = m_timer.GetElapsedTimeMilliseconds().ToFloat();

            if ( elapsedTime > s_defaultFadeTime + m_lifetime.ToFloat() + s_defaultFadeTime )
            {
                return Phase::Expired;
            }
            else if ( elapsedTime > s_defaultFadeTime + m_lifetime.ToFloat() )
            {
                return Phase::FadeOut;
            }
            else if ( elapsedTime > s_defaultFadeTime )
            {
                return Phase::Wait;
            }
            else
            {
                return Phase::FadeIn;
            }
        }

        float GetFadePercentage() const
        {
            Phase const phase = GetPhase();
            float const elapsedTime = m_timer.GetElapsedTimeMilliseconds().ToFloat();

            if ( phase == Phase::FadeIn )
            {
                return elapsedTime / s_defaultFadeTime;
            }
            else if ( phase == Phase::FadeOut )
            {
                return ( 1.f - ( ( elapsedTime - s_defaultFadeTime - m_lifetime.ToFloat() ) / s_defaultFadeTime ) );
            }

            return 1.f;
        }

        ImVec4 GetColor( float opacity = 1.0f ) const
        {
            switch ( m_type )
            {
                case Type::Success:
                return { 0, 255, 0, opacity }; // Green

                case Type::Warning:
                return { 255, 255, 0, opacity }; // Yellow

                case Type::Error:
                return { 255, 0, 0, opacity }; // Error

                case Type::Info:
                return { 0, 157, 255, opacity }; // Blue

                default:
                break;
            }

            //-------------------------------------------------------------------------

            EE_UNREACHABLE_CODE();
            return Style::s_colorText.ToFloat4();
        }

        char const* GetIcon() const
        {
            switch ( m_type )
            {
                case Type::Success:
                return EE_ICON_CHECK_CIRCLE;

                case Type::Warning:
                return EE_ICON_ALERT;

                case Type::Error:
                return EE_ICON_CLOSE_CIRCLE;

                case Type::Info:
                return EE_ICON_INFORMATION;

                default:
                break;
            }

            //-------------------------------------------------------------------------

            EE_UNREACHABLE_CODE();
            return nullptr;
        }

        char const* GetTitle() const
        {
            switch ( m_type )
            {
                case Type::Success:
                return "Success";

                case Type::Warning:
                return "Warning";

                case Type::Error:
                return "Error";

                case Type::Info:
                return "Info";

                default:
                break;
            }

            //-------------------------------------------------------------------------

            EE_UNREACHABLE_CODE();
            return nullptr;
        };

        char const* GetMessage() const { return m_message.c_str(); };

    private:

        Type                            m_type = Type::None;
        InlineString                    m_message;
        Milliseconds                    m_lifetime = s_defaultLifetime;
        Timer<EngineClock>              m_timer;
    };

    //-------------------------------------------------------------------------

    static TInlineVector<Notification, 100> g_notifications;

    void NotificationSystem::Initialize()
    {
        // Do Nothing
    }

    void NotificationSystem::Shutdown()
    {
        g_notifications.clear();
    }

    void NotificationSystem::Render()
    {
        constexpr static float const paddingX = 20.0f; // Bottom-left X padding
        constexpr static float const paddingY = 20.0f; // Bottom-left Y padding
        constexpr static float const paddingNotificationY = 10.0f; // Padding Y between each message

        //-------------------------------------------------------------------------

        ImGuiViewport const* pViewport = ImGui::GetMainViewport();
        ImVec2 const viewportSize = pViewport->Size;

        float notificationStartPosY = 0.f;
        for ( auto i = 0; i < g_notifications.size(); i++ )
        {
            Notification* pNotification = &g_notifications[i];

            // Remove notification if expired
            //-------------------------------------------------------------------------

            if ( pNotification->GetPhase() == Notification::Phase::Expired )
            {
                g_notifications.erase( g_notifications.begin() + i );
                i--;
                continue;
            }

            // Preparation
            //-------------------------------------------------------------------------

            // Get icon, title and other data
            char const* pIcon = pNotification->GetIcon();
            char const* pTitle = pNotification->GetTitle();
            char const* pMessage = pNotification->GetMessage();
            float const opacity = pNotification->GetFadePercentage(); // Get opacity based of the current phase
            ImVec4 const textColor = pNotification->GetColor( opacity );

            // Generate new unique name for this notification
            InlineString windowName( InlineString::CtorSprintf(), "##Notification%d", i );

            // Draw Notification
            //-------------------------------------------------------------------------

            ImGui::SetNextWindowBgAlpha( opacity );
            ImGui::SetNextWindowPos( pViewport->Pos + ImVec2( viewportSize.x - paddingX, viewportSize.y - paddingY - notificationStartPosY ), ImGuiCond_Always, ImVec2( 1.0f, 1.0f ) );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1 );
            ImGui::PushStyleColor( ImGuiCol_Border, textColor );
            if ( ImGui::Begin( windowName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing ) )
            {
                ImGui::PushTextWrapPos( viewportSize.x / 3.f ); // We want to support multi-line text, this will wrap the text after 1/3 of the screen width

                bool drawSeparator = false;

                // If an icon is set
                if ( pIcon != nullptr )
                {
                    ImGui::TextColored( textColor, pIcon );
                    drawSeparator = true;
                }

                if ( pTitle != nullptr )
                {
                    if ( pIcon != nullptr )
                    {
                        ImGui::SameLine();
                    }

                    ImGui::Text( pTitle ); // Render default title text (Success -> "Success", etc...)
                    drawSeparator = true;
                }

                // In case ANYTHING was rendered in the top, we want to add a small padding so the text (or icon) looks centered vertically
                if ( drawSeparator && pMessage != nullptr )
                {
                    ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5.f ); // Must be a better way to do this!!!!
                }

                // If a content is set
                if ( pMessage != nullptr )
                {
                    if ( drawSeparator )
                    {
                        ImGui::Separator();
                    }

                    ImGui::Text( pMessage ); // Render content text
                }

                ImGui::PopTextWrapPos();
            }

            // Save height for next notifications
            notificationStartPosY += ImGui::GetWindowHeight() + paddingNotificationY;

            ImGui::End();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
    }

    //-------------------------------------------------------------------------

    EE_BASE_API void NotifyInfo( const char* pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );
        InlineString str;
        str.append_sprintf_va_list( pFormat, args );
        g_notifications.emplace_back( Notification::Type::Info, str.c_str() );
        va_end( args );
    }

    EE_BASE_API void NotifySuccess( const char* pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );
        InlineString str;
        str.append_sprintf_va_list( pFormat, args );
        g_notifications.emplace_back( Notification::Type::Success, str.c_str() );
        va_end( args );
    }

    EE_BASE_API void NotifyWarning( const char* pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );
        InlineString str;
        str.append_sprintf_va_list( pFormat, args );
        g_notifications.emplace_back( Notification::Type::Warning, str.c_str() );
        va_end( args );
    }

    EE_BASE_API void NotifyError( const char* pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );
        InlineString str;
        str.append_sprintf_va_list( pFormat, args );
        g_notifications.emplace_back( Notification::Type::Error, str.c_str() );
        va_end( args );
    }
}