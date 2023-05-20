#pragma once

#include "Engine/_Module/API.h"
#include "System/TypeSystem/ReflectedType.h"
#include "System/Resource/IResource.h"
#include "System/Systems.h"

//-------------------------------------------------------------------------

namespace physx { class PxMaterial; }

//-------------------------------------------------------------------------

namespace EE::Physics
{
    enum class CombineMode
    {
        EE_REFLECT_ENUM

        Average = 0,
        Min,
        Multiply,
        Max,
    };

    //-------------------------------------------------------------------------
    // Material Settings
    //-------------------------------------------------------------------------
    // Serialized physical material settings

    struct EE_ENGINE_API MaterialSettings : public IReflectedType
    {
        EE_REFLECT_TYPE( MaterialSettings );
        EE_SERIALIZE( m_ID, m_dynamicFriction, m_staticFriction, m_restitution, m_frictionCombineMode, m_restitutionCombineMode );

        constexpr static char const* const s_defaultID = "Default";
        constexpr static float const s_defaultStaticFriction = 0.5f;
        constexpr static float const s_defaultDynamicFriction = 0.5f;
        constexpr static float const s_defaultRestitution = 0.5f;

    public:

        bool IsValid() const;

        EE_REFLECT()
        StringID                                m_ID;

        // The static friction coefficients - [0, FloatMax]
        EE_REFLECT();
        float                                   m_staticFriction = s_defaultStaticFriction;

        // The dynamic friction coefficients - [0, FloatMax]
        EE_REFLECT();
        float                                   m_dynamicFriction = s_defaultDynamicFriction;

        // The amount of restitution (bounciness) - [0,1]
        EE_REFLECT();
        float                                   m_restitution = s_defaultRestitution;

        // How material friction properties will be combined on collision
        EE_REFLECT();
        CombineMode                             m_frictionCombineMode = CombineMode::Average;
        
        // How material restitution properties will be combined on collision
        EE_REFLECT();
        CombineMode                             m_restitutionCombineMode = CombineMode::Average;
    };

    // Empty resource - acts as a placeholder for the actual data being loaded - see PhysicsMaterialDatabaseLoader for details
    class EE_ENGINE_API MaterialDatabase final : public Resource::IResource
    {
        EE_RESOURCE( 'pmdb', "Physics Material DB" );
        EE_SERIALIZE( m_materials );

        friend class PhysicsMaterialDatabaseCompiler;
        friend class PhysicsMaterialDatabaseLoader;

    public:

        bool IsValid() const override final { return true; }

    private:

        TVector<MaterialSettings>               m_materials;
    };

    //-------------------------------------------------------------------------
    // Physics Material Instance
    //-------------------------------------------------------------------------
    // Physics material instance, created from the serialized settings

    struct EE_ENGINE_API MaterialInstance
    {
    public:

        MaterialInstance( StringID ID, physx::PxMaterial* pMaterial )
            : m_ID( ID )
            , m_pMaterial( pMaterial )
        {
            EE_ASSERT( ID.IsValid() && pMaterial != nullptr );
        }

    public:

        StringID                                m_ID;
        physx::PxMaterial*                      m_pMaterial = nullptr;
    };

    //-------------------------------------------------------------------------
    // Physics Material ID
    //-------------------------------------------------------------------------

    // Needed for a custom property editor in the tools
    struct EE_ENGINE_API MaterialID : public IReflectedType
    {
        EE_REFLECT_TYPE( MaterialID );
        EE_SERIALIZE( m_ID );

        MaterialID() = default;
        MaterialID( StringID ID ) : m_ID( ID ) {}

    public:

        EE_REFLECT();
        StringID                                        m_ID = StringID( MaterialSettings::s_defaultID );
    };

    //-------------------------------------------------------------------------
    // Physics Material Database
    //-------------------------------------------------------------------------

    class EE_ENGINE_API MaterialRegistry final : public ISystem
    {
        friend class PhysicsDebugView;

    public:

        EE_SYSTEM( MaterialRegistry );

    public:

        void Initialize();
        void Shutdown();

        void RegisterMaterials( TVector<MaterialSettings> const& materials );
        void UnregisterMaterials( TVector<MaterialSettings> const& materials );

        physx::PxMaterial* GetDefaultMaterial() const { return m_pDefaultMaterial; }
        physx::PxMaterial* GetMaterial( StringID materialID ) const;

        EE_FORCE_INLINE physx::PxMaterial* GetMaterial( MaterialID materialID ) const { return GetMaterial( materialID.m_ID ); }

    private:

        THashMap<StringID, MaterialInstance>            m_materials;
        StringID                                        m_defaultMaterialID;
        physx::PxMaterial*                              m_pDefaultMaterial = nullptr;
    };
}