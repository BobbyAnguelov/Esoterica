#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Render/RenderProxies.h"
#include "Engine/Render/RenderDecalMaterial.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/BitFlags.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderWorldSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DecalComponent final : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( DecalComponent );

        friend class RenderWorldSystem;

    public:

        using SpatialEntityComponent::SpatialEntityComponent;

        // Material
        //-------------------------------------------------------------------------

        inline DecalMaterial const* GetDecalMaterial() const { return m_decalMaterial.IsLoaded() ? m_decalMaterial.GetPtr() : nullptr; }

        //-------------------------------------------------------------------------

        virtual void OnWorldTransformUpdated() override;

        virtual bool SupportsNonUniformScale() const override { return true; }

    protected:

        virtual void OnNonUniformScaleChanged() override;

        //-------------------------------------------------------------------------

    private:

        virtual Float3* GetNonUniformScaleForEdit() override { return &m_nonUniformScale; }

        virtual OBB CalculateLocalBounds() const override;

        //-------------------------------------------------------------------------

        void WriteDecalTransform();

    private:

        EE_REFLECT( Category = "Decal" );
        TResourcePtr<DecalMaterial>                     m_decalMaterial;

        EE_REFLECT( Category = "Decal" );
        Float3                                          m_nonUniformScale = Float3::One;

        // Internal renderer data
        //-------------------------------------------------------------------------

        DecalInstanceProxy                              m_decalInstanceProxy = {};
    };
}
