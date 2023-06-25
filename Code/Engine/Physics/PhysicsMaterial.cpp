#include "PhysicsMaterial.h"
#include "System/Math/Math.h"
#include "Physics.h"

#include <PxMaterial.h>

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool MaterialSettings::IsValid() const
    {
        static StringID const defaultMaterialID( s_defaultID );

        if ( !m_ID.IsValid() || m_ID == StringID( defaultMaterialID ) )
        {
            return false;
        }

        if ( !Math::IsInRangeInclusive( m_staticFriction, 0.0f, FLT_MAX ) || !Math::IsInRangeInclusive( m_dynamicFriction, 0.0f, FLT_MAX ) )
        {
            return false;
        }

        return Math::IsInRangeInclusive( m_restitution, 0.0f, 1.0f );
    }

    //-------------------------------------------------------------------------

    void MaterialRegistry::Initialize()
    {
        auto pPhysics = Core::GetPxPhysics();

        EE_ASSERT( m_materials.empty() );

        // Create default physics material
        m_defaultMaterialID = StringID( MaterialSettings::s_defaultID );
        m_pDefaultMaterial = pPhysics->createMaterial( MaterialSettings::s_defaultStaticFriction, MaterialSettings::s_defaultDynamicFriction, MaterialSettings::s_defaultRestitution );
        reinterpret_cast<uintptr_t&>( m_pDefaultMaterial->userData ) = m_defaultMaterialID.ToUint();
        m_materials.insert( TPair<StringID, MaterialInstance>( m_defaultMaterialID, MaterialInstance( m_defaultMaterialID, m_pDefaultMaterial ) ) );
    }

    void MaterialRegistry::Shutdown()
    {
        for ( auto& materialPair : m_materials )
        {
            materialPair.second.m_pMaterial->release();
            materialPair.second.m_pMaterial = nullptr;
        }

        m_materials.clear();

        //-------------------------------------------------------------------------

        m_pDefaultMaterial = nullptr;
        m_defaultMaterialID = StringID();
    }

    void MaterialRegistry::RegisterMaterials( TVector<MaterialSettings> const& materials )
    {
        auto pPhysics = Core::GetPxPhysics();

        EE_ASSERT( m_defaultMaterialID.IsValid() );

        // Create physX materials
        physx::PxMaterial* pPxMaterial = nullptr;
        for ( auto const& materialSettings : materials )
        {
            EE_ASSERT( materialSettings.m_ID != m_defaultMaterialID );
            EE_ASSERT( m_materials.find( materialSettings.m_ID ) == m_materials.end() );

            pPxMaterial = pPhysics->createMaterial( materialSettings.m_staticFriction, materialSettings.m_dynamicFriction, materialSettings.m_restitution );
            pPxMaterial->setFrictionCombineMode( (physx::PxCombineMode::Enum) materialSettings.m_frictionCombineMode );
            pPxMaterial->setRestitutionCombineMode( (physx::PxCombineMode::Enum) materialSettings.m_restitutionCombineMode );
            reinterpret_cast<uintptr_t&>( m_pDefaultMaterial->userData ) = materialSettings.m_ID.ToUint();
            m_materials.insert( TPair<StringID, MaterialInstance>( materialSettings.m_ID, MaterialInstance( materialSettings.m_ID, pPxMaterial ) ) );
        }
    }

    void MaterialRegistry::UnregisterMaterials( TVector<MaterialSettings> const& materials )
    {
        EE_ASSERT( m_defaultMaterialID.IsValid() );

        for ( auto const& materialSettings : materials )
        {
            EE_ASSERT( materialSettings.m_ID != m_defaultMaterialID );
            auto foundMaterialPairIter = m_materials.find( materialSettings.m_ID );
            EE_ASSERT( foundMaterialPairIter != m_materials.end() );

            // Remove material
            foundMaterialPairIter->second.m_pMaterial->release();
            foundMaterialPairIter->second.m_pMaterial = nullptr;
            m_materials.erase( foundMaterialPairIter );
        }
    }

    physx::PxMaterial* MaterialRegistry::GetMaterial( StringID materialID ) const
    {
        EE_ASSERT( materialID.IsValid() );

        auto foundMaterialPairIter = m_materials.find( materialID );
        if ( foundMaterialPairIter != m_materials.end() )
        {
            return foundMaterialPairIter->second.m_pMaterial;
        }

        EE_LOG_WARNING( "Physics", "Physics System", "Failed to find physic material with ID: %s", materialID.c_str() );
        return nullptr;
    }
}