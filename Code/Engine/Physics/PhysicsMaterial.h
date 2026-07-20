#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Physics/Physics.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Resource/IResource.h"
#include "Base/Systems.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Material Settings
    //-------------------------------------------------------------------------
    // Serialized physical material settings

    struct EE_ENGINE_API Material : public IReflectedType
    {
        EE_REFLECT_TYPE( Material );
        EE_SERIALIZE( m_ID, m_friction, m_restitution, m_rollingResistance );

        friend class PhysicsMaterialDatabaseLoader;

        static inline StaticStringID const s_defaultID = StaticStringID( "Default" );

    public:

        bool IsValid() const;

        b3SurfaceMaterial GetB3D() const
        { 
            b3SurfaceMaterial material;
            material.friction = m_friction;
            material.restitution = m_restitution;
            material.rollingResistance = m_rollingResistance;
            material.userMaterialId = m_ID.ToUint();
            return material;
        }

    public:

        EE_REFLECT()
        StringID                                m_ID;

        // The static friction coefficients - [0, 1]
        EE_REFLECT( Min = "0.0f", Max = "1.0f" );
        float                                   m_friction = 0.5f;

        // The amount of restitution (bounciness) - [0,1]
        EE_REFLECT( Min = "0.0f", Max = "1.0f" );
        float                                   m_restitution = 0.5f;

        // The rolling resistance for spheres and capsules - [0, 1]
        EE_REFLECT( Min = "0.0f", Max = "1.0f" );
        float                                   m_rollingResistance = 0.5f;
    };

    //-------------------------------------------------------------------------

    // Empty resource - acts as a placeholder for the actual data being loaded - see PhysicsMaterialDatabaseLoader for details
    class EE_ENGINE_API MaterialDatabase final : public Resource::IResource
    {
        EE_RESOURCE( "pmdb", "Physics Material DB", Colors::RoyalBlue, 2, false );
        EE_SERIALIZE( m_materials );

        friend class PhysicsMaterialDatabaseCompiler;
        friend class PhysicsMaterialDatabaseLoader;

    public:

        bool IsValid() const override final { return true; }

    private:

        TVector<Material>                       m_materials;
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
        StringID                                m_ID = StringID( Material::s_defaultID );
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

        void RegisterMaterials( TVector<Material> const& materials );
        void UnregisterMaterials( TVector<Material> const& materials );

        b3SurfaceMaterial const& GetDefaultMaterial() const { return m_defaultMaterial; }
        b3SurfaceMaterial const& GetMaterial( StringID materialID ) const;
        EE_FORCE_INLINE b3SurfaceMaterial const& GetMaterial( MaterialID materialID ) const { return GetMaterial( materialID.m_ID ); }

    private:

        THashMap<StringID, b3SurfaceMaterial>   m_materials;
        StringID                                m_defaultMaterialID;
        b3SurfaceMaterial                       m_defaultMaterial = {};
    };
}