#include "Component_GameInput.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void GameInputComponent::Initialize()
    {
        EntityComponent::Initialize();

        if ( HasInputState() )
        {
            GetInputState()->RegisterInputs();
        }
    }

    void GameInputComponent::Shutdown()
    {
        if ( HasInputState() )
        {
            GetInputState()->UnregisterInputs();
        }

        EntityComponent::Shutdown();
    }

    void GameInputComponent::Update( EntityWorldUpdateContext const& ctx )
    {
        if ( !m_isEnabled )
        {
            return;
        }

        EE_ASSERT( ctx.GetUpdateStage() == UpdateStage::GameSetup );

        if ( HasInputState() )
        {
            auto pInputSystem = ctx.GetSystem<InputSystem>();
            GetInputState()->Update( pInputSystem, ctx.GetRawDeltaTime(), ctx.GetScaledDeltaTime() );
        }
    }
}