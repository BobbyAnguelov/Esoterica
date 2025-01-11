#include "Component_StaticMesh.h"
#include "Engine/Entity/EntityLog.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    OBB StaticMeshComponent::CalculateLocalBounds() const
    {
        if ( HasMeshResourceSet() )
        {
            OBB scaledMeshBounds = m_mesh->GetBounds();
            scaledMeshBounds.m_center *= m_localScale;
            scaledMeshBounds.m_extents *= m_localScale;
            return scaledMeshBounds;
        }
        else
        {
            return MeshComponent::CalculateLocalBounds();
        }
    }

    TVector<TResourcePtr<Render::Material>> const& StaticMeshComponent::GetDefaultMaterials() const
    {
        EE_ASSERT( IsInitialized() && HasMeshResourceSet() );
        return m_mesh->GetMaterials();
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void StaticMeshComponent::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        MeshComponent::PostPropertyEdit( pPropertyEdited );

        if ( Math::IsNearZero( m_localScale.m_x ) ) m_localScale.m_x = 0.1f;
        if ( Math::IsNearZero( m_localScale.m_y ) ) m_localScale.m_y = 0.1f;
        if ( Math::IsNearZero( m_localScale.m_z ) ) m_localScale.m_z = 0.1f;
    }
    #endif
}