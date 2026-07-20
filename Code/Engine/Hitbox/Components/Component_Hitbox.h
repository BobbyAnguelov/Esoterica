#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Hitbox/Hitbox_Instance.h"
#include "Engine/Entity/EntityComponent.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API HitboxComponent : public EntityComponent
    {
        EE_ENTITY_COMPONENT( HitboxComponent );

    public:

        // Hitbox
        //-------------------------------------------------------------------------

        inline bool HasHitboxDefinitionSet() const { return m_hitboxDefinition.IsSet(); }

        inline void SetHitbox( ResourceID hitboxDefinitionResourceID )
        {
            EE_ASSERT( IsUnloaded() );
            EE_ASSERT( hitboxDefinitionResourceID.IsValid() && hitboxDefinitionResourceID.GetResourceTypeID() == HitboxDefinition::GetStaticResourceTypeID() );
            m_hitboxDefinition = hitboxDefinitionResourceID;
        }

        inline bool HasHitbox() const { return m_pHitbox != nullptr; }

        inline Hitbox const* GetHitbox() const
        {
            EE_ASSERT( m_pHitbox != nullptr );
            return m_pHitbox;
        }

        inline Hitbox* GetHitbox()
        {
            EE_ASSERT( m_pHitbox != nullptr );
            return m_pHitbox;
        }

        inline void Update( Seconds deltaTime, SpatialEntityComponent const* pComponent )
        {
            EE_ASSERT( m_pHitbox != nullptr );
            m_pHitbox->Update( deltaTime, pComponent );
        }

        inline AABB const& GetBounds() const
        {
            EE_ASSERT( m_pHitbox != nullptr );
            return m_pHitbox->GetBounds();
        }

        // Runtime
        //-------------------------------------------------------------------------

        inline bool IsEnabled() const { return m_isEnabled; }
        void SetEnabled( bool isEnabled ) { m_isEnabled = isEnabled; }

    private:

        virtual void Initialize() override;
        virtual void Shutdown() override;

    private:

        EE_REFLECT( Category = "Gameplay" );
        TResourcePtr<HitboxDefinition>          m_hitboxDefinition;

        Hitbox*                                 m_pHitbox = nullptr;
        bool                                    m_isEnabled = true;
    };
}