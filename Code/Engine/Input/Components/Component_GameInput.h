#pragma once

#include "Engine/Input/GameInput.h"
#include "Engine/Entity/EntityComponent.h"
#include "Base/TypeSystem/TypeInstance.h"

//-------------------------------------------------------------------------
// Game Input Component
//-------------------------------------------------------------------------
// This component manages all game input state for an entity

namespace EE { class EntityWorldUpdateContext; }

//-------------------------------------------------------------------------

namespace EE::Input
{
    class EE_ENGINE_API GameInputComponent final : public EntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( GameInputComponent );

    public:

        inline GameInputComponent() = default;

        inline bool IsEnabled() const { return m_isEnabled; }
        inline void SetEnabled( bool isEnabled ) { m_isEnabled = isEnabled; }

        void Update( EntityWorldUpdateContext const& ctx );

        // Input State
        //-------------------------------------------------------------------------

        inline bool HasInputState() const { return m_inputState.IsSet(); }

        Input::GameInputState* GetInputState() { return m_inputState.Get(); }

        Input::GameInputState const* GetInputState() const { return m_inputState.Get(); }

        template<typename T>
        T* GetInputState() { return m_inputState.GetAs<T>(); }

        template<typename T>
        T const* GetInputState() const { return m_inputState.GetAs<T>(); }

    private:

        virtual void Initialize() override;
        virtual void Shutdown() override;

    protected:

        EE_REFLECT();
        TTypeInstance<Input::GameInputState>    m_inputState;

    private:

        bool                                    m_isEnabled = true;
    };
}