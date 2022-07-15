#pragma once

#include "Engine/_Module/API.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Resource/IResource.h"
#include "PxMaterial.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Physics Material
    //-------------------------------------------------------------------------
    // Physics material instance, created from the serialized settings

    struct EE_ENGINE_API PhysicsMaterial
    {
        constexpr static char const* const DefaultID = "Default";
        constexpr static float const DefaultStaticFriction = 0.5f;
        constexpr static float const DefaultDynamicFriction = 0.5f;
        constexpr static float const DefaultRestitution = 0.5f;

    public:

        PhysicsMaterial( StringID ID, physx::PxMaterial* pMaterial )
            : m_ID( ID )
            , m_pMaterial( pMaterial )
        {
            EE_ASSERT( ID.IsValid() && pMaterial != nullptr );
        }

    public:

        StringID                                m_ID;
        physx::PxMaterial*                      m_pMaterial = nullptr;
    };

    enum class PhysicsCombineMode
    {
        EE_REGISTER_ENUM

        Average = physx::PxCombineMode::eAVERAGE,
        Min = physx::PxCombineMode::eMIN,
        Multiply = physx::PxCombineMode::eMULTIPLY,
        Max = physx::PxCombineMode::eMAX,
    };

    //-------------------------------------------------------------------------
    // Material Settings
    //-------------------------------------------------------------------------
    // Serialized physical material settings

    struct EE_ENGINE_API PhysicsMaterialSettings : public IRegisteredType
    {
        EE_REGISTER_TYPE( PhysicsMaterialSettings );
        EE_SERIALIZE( m_ID, m_dynamicFriction, m_staticFriction, m_restitution, m_frictionCombineMode, m_restitutionCombineMode );

        bool IsValid() const;

        EE_EXPOSE StringID                         m_ID;

        // The friction coefficients - [0, FloatMax]
        EE_EXPOSE float                            m_staticFriction = PhysicsMaterial::DefaultStaticFriction;
        EE_EXPOSE float                            m_dynamicFriction = PhysicsMaterial::DefaultDynamicFriction;

        // The amount of restitution (bounciness) - [0,1]
        EE_EXPOSE float                            m_restitution = PhysicsMaterial::DefaultRestitution;

        // How material properties will be combined on collision
        EE_EXPOSE PhysicsCombineMode               m_frictionCombineMode = PhysicsCombineMode::Average;
        EE_EXPOSE PhysicsCombineMode               m_restitutionCombineMode = PhysicsCombineMode::Average;
    };

    //-------------------------------------------------------------------------
    // Physics Material Database
    //-------------------------------------------------------------------------
    // Empty resource - acts as a placeholder for the actual data being loaded - see PhysicsMaterialDatabaseLoader for details

    class EE_ENGINE_API PhysicsMaterialDatabase final : public Resource::IResource
    {
        EE_REGISTER_RESOURCE( 'pmdb', "Physics Material DB" );
        friend class PhysicsMaterialDatabaseCompiler;
        friend class PhysicsMaterialDatabaseLoader;

        EE_SERIALIZE();

    public:

        bool IsValid() const override final { return true; }
    };
}