#include "WorldSystem_GameInput.h"
#include "Engine/Input/Components/Component_GameInput.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/Entity.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void GameInputSystem::ShutdownSystem()
    {
        EE_ASSERT( m_inputComponents.empty() );
    }

    //-------------------------------------------------------------------------

    void GameInputSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pInputComponent = TryCast<GameInputComponent>( pComponent ) )
        {
            m_inputComponents.emplace_back( pInputComponent );
        }
    }

    void GameInputSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pInputComponent = TryCast<GameInputComponent>( pComponent ) )
        {
            m_inputComponents.erase_first( pInputComponent );
        }
    }

    //-------------------------------------------------------------------------

    void GameInputSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        EE_ASSERT( ctx.GetUpdateStage() == UpdateStage::GameSetup );

        if ( !ctx.IsGameWorld() )
        {
            return;
        }

        if ( m_isGameInputEnabled )
        {
            for ( GameInputComponent* pInputComponent : m_inputComponents )
            {
                pInputComponent->Update( ctx );
            }
        }
    }
}