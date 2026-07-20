#pragma once
#include "Base/Encoding/Hash.h"
#include "PlayerActionContext.h"

#if EE_DEVELOPMENT_TOOLS
#include "imgui.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    //-------------------------------------------------------------------------
    // The player action state
    //-------------------------------------------------------------------------
    // This defines a discrete action the player is currently undertaking i.e. moving, shooting, reloading, etc...

    class PlayerAction
    {
    public:

        enum class Status : uint8_t
        {
            Interruptible,      // Running and allow transitions
            Uninterruptible,    // Running but block transitions
            Completed           // Finished
        };

        enum class StopReason : uint8_t
        {
            Completed,
            Interrupted
        };

    public:

        virtual ~PlayerAction() = default;

        // Get the ID for this action
        virtual uint64_t GetActionID() const = 0;

        // Is this action active
        inline bool IsActive() const { return m_isActive; }

        // Try to start this action - this is where you check all the start preconditions
        inline bool TryStart( PlayerActionContext const& ctx ) 
        {
            EE_ASSERT( !m_isActive );
            if ( TryStartInternal( ctx ) )
            {
                m_isActive = true;
                return true;
            }

            return false;
        }

        // Called to update this action, this will be called directly after the try start if it succeeds
        inline Status Update( PlayerActionContext const& ctx, bool isFirstUpdate )
        {
            EE_ASSERT( m_isActive );
            return UpdateInternal( ctx, isFirstUpdate );
        }

        // Called to stop this action
        inline void Stop( PlayerActionContext const& ctx, StopReason reason )
        {
            EE_ASSERT( m_isActive );
            StopInternal( ctx, reason );
            m_isActive = false;
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Returns a friendly name for the action
        virtual char const* GetName() const = 0;

        // Override this function to draw custom imgui controls in the action debugger UI
        virtual void DrawDebugUI() { ImGui::Text( "No Debug" ); }
        #endif

    protected:

        // Try to start this action - this is where you check all the start preconditions
        virtual bool TryStartInternal( PlayerActionContext const& ctx ) { return true; }

        // Called to update this action, this will be called directly after the try start if it succeeds
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) = 0;

        // Called to stop this action
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) = 0;

    private:

        bool m_isActive = false;
    };

    //-------------------------------------------------------------------------

    // Needed for type safety
    class OverlayPlayerAction : public PlayerAction
    {

    };
}

//-------------------------------------------------------------------------

#define EE_PLAYER_ACTION_ID( TypeName ) \
constexpr static uint64_t const s_gameplayStateID = Hash::FNV1a::GetHash64( #TypeName ); \
virtual uint64_t GetActionID() const override final { return TypeName::s_gameplayStateID; }\
EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( char const* GetName() const override final { return #TypeName; } )