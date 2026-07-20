#include "PhysicsMaterial.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool Material::IsValid() const
    {
        static StringID const defaultMaterialID( s_defaultID );

        if ( !m_ID.IsValid() || m_ID == s_defaultID )
        {
            return false;
        }

        if ( !Math::IsInRangeInclusive( m_friction, 0.0f, 1.0f ) ||
             !Math::IsInRangeInclusive( m_restitution, 0.0f, 1.0f ) ||
             !Math::IsInRangeInclusive( m_rollingResistance, 0.0f, 1.0f ) )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void MaterialRegistry::Initialize()
    {
        EE_ASSERT( m_materials.empty() );

        m_defaultMaterialID = Material::s_defaultID;
        m_defaultMaterial.friction = 0.5f;
        m_defaultMaterial.restitution = 0.5f;
        m_defaultMaterial.rollingResistance = 0.5f;
        m_defaultMaterial.userMaterialId = m_defaultMaterialID.ToUint();

        m_materials.insert( TPair<StringID, b3SurfaceMaterial>( m_defaultMaterialID, m_defaultMaterial ) );
    }

    void MaterialRegistry::Shutdown()
    {
        m_materials.clear();
        m_defaultMaterialID = StringID();
    }

    void MaterialRegistry::RegisterMaterials( TVector<Material> const& materials )
    {
        EE_ASSERT( m_defaultMaterialID.IsValid() );

        for ( auto const& material : materials )
        {
            EE_ASSERT( m_materials.find( material.m_ID ) == m_materials.end() );
            m_materials.try_emplace( material.m_ID, material.GetB3D() );
        }
    }

    void MaterialRegistry::UnregisterMaterials( TVector<Material> const& materials )
    {
        EE_ASSERT( m_defaultMaterialID.IsValid() );

        for ( auto const& materialSettings : materials )
        {
            EE_ASSERT( materialSettings.m_ID != m_defaultMaterialID );
            auto foundMaterialPairIter = m_materials.find( materialSettings.m_ID );
            EE_ASSERT( foundMaterialPairIter != m_materials.end() );
            m_materials.erase( foundMaterialPairIter );
        }
    }

    b3SurfaceMaterial const& MaterialRegistry::GetMaterial( StringID materialID ) const
    {
        EE_ASSERT( materialID.IsValid() );

        auto foundMaterialPairIter = m_materials.find( materialID );
        if ( foundMaterialPairIter != m_materials.end() )
        {
            return foundMaterialPairIter->second;
        }

        EE_LOG_WARNING( LogCategory::Physics, "Physics System", "Failed to find physic material with ID: %s", materialID.c_str() );
        return m_defaultMaterial;
    }
}