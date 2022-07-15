#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntityWorldSystem.h"
#include "System/Types/IDVector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class CoverVolumeComponent;

    //-------------------------------------------------------------------------

    class EE_GAME_API CoverManager : public IWorldEntitySystem
    {
        friend class CoverDebugView;

    public:

        EE_REGISTER_TYPE( CoverManager );
        EE_ENTITY_WORLD_SYSTEM( CoverManager, RequiresUpdate( UpdateStage::PrePhysics ) );

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TIDVector<ComponentID, CoverVolumeComponent*>      m_coverVolumes;
    };
}