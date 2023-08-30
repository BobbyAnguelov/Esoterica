#pragma once
#include "Base/Encoding/Hash.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"

#if EE_DEVELOPMENT_TOOLS
#include "imgui.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    class EntityComponent;
    namespace Physics { class CharacterComponent; class PhysicsWorld; }
    namespace Input { class InputState; }
    namespace Animation { class GraphController; }
}

//-------------------------------------------------------------------------

namespace EE::Player
{
    class MainPlayerComponent;
    class AnimationController;
    class CameraController;

    //-------------------------------------------------------------------------
    // The context for all player actions
    //-------------------------------------------------------------------------
    // Provides the common set of systems and components needed for player actions

    struct ActionContext
    {
        ~ActionContext();

        bool IsValid() const;

        template<typename T>
        T* GetComponentByType() const
        {
            static_assert( std::is_base_of<EntityComponent, T>::value, "T must be a component type" );
            for ( auto pComponent : m_components )
            {
                if ( auto pCastComponent = TryCast<T>( pComponent ) )
                {
                    return pCastComponent;
                }
            }

            return nullptr;
        }

        // Forwarding helper functions
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Seconds GetDeltaTime() const { return m_pEntityWorldUpdateContext->GetDeltaTime(); }
        template<typename T> inline T* GetWorldSystem() const { return m_pEntityWorldUpdateContext->GetWorldSystem<T>(); }
        template<typename T> inline T* GetSystem() const { return m_pEntityWorldUpdateContext->GetSystem<T>(); }
        template<typename T> inline T* GetAnimSubGraphController() const { return m_pAnimationController->GetSubGraphController<T>(); }

        #if EE_DEVELOPMENT_TOOLS
        Drawing::DrawContext GetDrawingContext() const;
        #endif

    public:

        EntityWorldUpdateContext const*             m_pEntityWorldUpdateContext = nullptr;
        Input::InputState const*                    m_pInputState = nullptr;
        Physics::PhysicsWorld*                      m_pPhysicsWorld = nullptr;

        MainPlayerComponent*                        m_pPlayerComponent = nullptr;
        Physics::CharacterComponent*                m_pCharacterComponent = nullptr;
        CameraController*                           m_pCameraController = nullptr;
        AnimationController*                        m_pAnimationController = nullptr;
        TInlineVector<EntityComponent*, 10>         m_components;
    };

    //-------------------------------------------------------------------------
    // The player action state
    //-------------------------------------------------------------------------
    // This defines a discrete action the player is currently undertaking i.e. moving, shooting, reloading, etc...

    class Action
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

        virtual ~Action() = default;

        // Get the ID for this action
        virtual uint32_t GetActionID() const = 0;

        // Is this action active
        inline bool IsActive() const { return m_isActive; }

        // Try to start this action - this is where you check all the start preconditions
        inline bool TryStart( ActionContext const& ctx ) 
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
        inline Status Update( ActionContext const& ctx )
        {
            EE_ASSERT( m_isActive );
            return UpdateInternal( ctx );
        }

        // Called to stop this action
        inline void Stop( ActionContext const& ctx, StopReason reason )
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
        virtual bool TryStartInternal( ActionContext const& ctx ) { return true; }

        // Called to update this action, this will be called directly after the try start if it succeeds
        virtual Status UpdateInternal( ActionContext const& ctx ) = 0;

        // Called to stop this action
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) = 0;

    private:

        bool    m_isActive = false;
    };

    //-------------------------------------------------------------------------

    // Needed for type safety
    class OverlayAction : public Action
    {

    };
}

//-------------------------------------------------------------------------

#define EE_PLAYER_ACTION_ID( TypeName ) \
constexpr static uint32_t const s_gameplayStateID = Hash::FNV1a::GetHash32( #TypeName ); \
virtual uint32_t GetActionID() const override final { return TypeName::s_gameplayStateID; }\
EE_DEVELOPMENT_TOOLS_LINE_IN_MACRO( char const* GetName() const override final { return #TypeName; } )