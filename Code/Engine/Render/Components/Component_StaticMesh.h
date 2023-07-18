#pragma once

#include "Component_RenderMesh.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Base/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class Mobility
    {
        EE_REFLECT_ENUM

        Static = 0,
        Dynamic = 1
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API StaticMeshComponent final : public MeshComponent
    {
        EE_ENTITY_COMPONENT( StaticMeshComponent );

        static TEvent<StaticMeshComponent*> s_mobilityChangedEvent; // Fired whenever we switch mobility 
        static TEvent<StaticMeshComponent*> s_staticMobilityTransformUpdatedEvent; // Fired whenever a "static" component is moved

    public:

        inline static TEventHandle<StaticMeshComponent*> OnStaticMobilityTransformUpdated() { return s_staticMobilityTransformUpdatedEvent; }
        inline static TEventHandle<StaticMeshComponent*> OnMobilityChanged() { return s_mobilityChangedEvent; }

    public:

        using MeshComponent::MeshComponent;

        // Local Scale
        //-------------------------------------------------------------------------

        virtual Float3 const& GetLocalScale() const override { return m_localScale; }
        virtual bool SupportsLocalScale() const override { return true; }

        // Mesh Data
        //-------------------------------------------------------------------------

        virtual bool HasMeshResourceSet() const override final { return m_mesh.IsSet(); }

        inline void SetMesh( ResourceID meshResourceID )
        {
            EE_ASSERT( IsUnloaded() );
            EE_ASSERT( meshResourceID.IsValid() );
            m_mesh = meshResourceID;
        }

        inline StaticMesh const* GetMesh() const
        {
            EE_ASSERT( m_mesh != nullptr && m_mesh->IsValid() );
            return m_mesh.GetPtr();
        }

        // Mobility
        //-------------------------------------------------------------------------

        inline Mobility GetMobility() const { return m_mobility; }
        void ChangeMobility( Mobility newMobility );

        virtual TVector<TResourcePtr<Material>> const& GetDefaultMaterials() const override;

    protected:

        virtual OBB CalculateLocalBounds() const override final;
        virtual void OnWorldTransformUpdated() override final;

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

    protected:

        // A local scale that doesnt propagate but that can allow for non-uniform scaling of meshes
        EE_REFLECT() Float3                                m_localScale = Float3::One;

    private:

        // The mesh resource for this component
        EE_REFLECT() TResourcePtr<StaticMesh>              m_mesh;

        // Is this component expected to be static (immovable) or dynamic (allowed to move)
        EE_REFLECT() Mobility                              m_mobility = Mobility::Static;
    };
}