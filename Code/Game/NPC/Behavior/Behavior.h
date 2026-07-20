#pragma once
#include "Base/Encoding/Hash.h"
#include "BehaviorContext.h"
#include "Game/NPC/Animation/NPCAnimationController.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"

#if EE_DEVELOPMENT_TOOLS
#include "imgui.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    class EntityComponent;
    namespace Physics { class CharacterComponent; class PhysicsWorld; }
    namespace Navmesh { class NavmeshWorldSystem; }
    namespace Drawing { class DrawContext; }
}

//-------------------------------------------------------------------------

namespace EE
{
    class NPCComponent;
    class NPCCharacterController;
    class NPCAnimationController;

    //-------------------------------------------------------------------------
    // An NPC behavior
    //-------------------------------------------------------------------------
    // This defines a behavior i.e. a sequence of actions to achieve a specified goal

    class Behavior
    {
    public:

        enum class Status : uint8_t
        {
            Running,
            Completed,
            Failed
        };

        enum class StopReason : uint8_t
        {
            Completed,
            Interrupted
        };

    public:

        virtual ~Behavior() = default;

        // Get the ID for this action
        virtual uint64_t GetBehaviorID() const = 0;

        // Is this action active
        inline bool IsActive() const { return m_isActive; }

        // Try to start this action - this is where you check all the start preconditions
        inline void Start( BehaviorContext const& ctx )
        {
            EE_ASSERT( !m_isActive );
            StartInternal( ctx );
            m_isActive = true;
        }

        // Called to update this action, this will be called directly after the try start if it succeeds
        inline Status Update( BehaviorContext const& ctx )
        {
            EE_ASSERT( m_isActive );
            return UpdateInternal( ctx );
        }

        // Called to stop this action
        inline void Stop( BehaviorContext const& ctx, StopReason reason )
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

        // Called to start this action
        virtual void StartInternal( BehaviorContext const& ctx ) = 0;

        // Called to update this action, this will be called directly after the try start if it succeeds
        virtual Status UpdateInternal( BehaviorContext const& ctx ) = 0;

        // Called to stop this action
        virtual void StopInternal( BehaviorContext const& ctx, StopReason reason ) = 0;

    private:

        bool    m_isActive = false;
    };
}

//-------------------------------------------------------------------------

#define EE_BEHAVIOR_ID( TypeName ) \
constexpr static uint64_t const s_behaviorID = Hash::FNV1a::GetHash64( #TypeName ); \
virtual uint64_t GetBehaviorID() const override final { return TypeName::s_behaviorID; }\
EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( char const* GetName() const override final { return #TypeName; } )