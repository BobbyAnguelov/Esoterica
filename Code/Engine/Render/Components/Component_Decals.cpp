#include "Component_Decals.h"
#include "Engine/Render/Device/DeviceRenderWorld.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void DecalComponent::OnWorldTransformUpdated()
    {
        WriteDecalTransform();
    }

    //-------------------------------------------------------------------------

    void DecalComponent::WriteDecalTransform()
    {
        if ( !m_decalInstanceProxy.IsValid() )
        {
            return;
        }

        DecalMaterial const* const pDecalMaterial = GetDecalMaterial();
        if ( pDecalMaterial == nullptr )
        {
            return;
        }

        Transform const& worldTransform = GetWorldTransform();
        Float3 const worldScale = Float3( worldTransform.GetScale() ) * GetWorldNonUniformScale();
        Float3 const inverseScale( 1.0F / worldScale.m_x, 1.0F / worldScale.m_y, 1.0F / worldScale.m_z );
        uint32_t const packedEmissiveColor = pDecalMaterial->GetEmissiveColor().ToLinear().ToUInt32();

        m_decalInstanceProxy.WriteDecal
        (
            worldTransform, inverseScale,
            pDecalMaterial->GetTint(),
            pDecalMaterial->GetOpacity(),
            pDecalMaterial->GetTiling(),
            pDecalMaterial->GetShapeRadius(),
            pDecalMaterial->GetHeightOffset(),
            packedEmissiveColor
        );
    }

    //-------------------------------------------------------------------------

    void DecalComponent::OnNonUniformScaleChanged()
    {
        WriteDecalTransform();
    }

    //-------------------------------------------------------------------------

    OBB DecalComponent::CalculateLocalBounds() const
    {
        return OBB( Vector::Zero, Vector( Float3( 0.5F ) * GetWorldNonUniformScale() ) );
    }
}
