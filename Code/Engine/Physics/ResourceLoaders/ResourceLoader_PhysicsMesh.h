#pragma once

#include "Engine/_Module/API.h"
#include "System/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsSystem;

    //-------------------------------------------------------------------------

    class PhysicsMeshLoader : public Resource::ResourceLoader
    {
    public:

        PhysicsMeshLoader();
        ~PhysicsMeshLoader() { EE_ASSERT( m_pPhysicsSystem == nullptr ); }

        void SetPhysicsSystemPtr( PhysicsSystem* pPhysicsSystem );
        void ClearPhysicsSystemPtr() { m_pPhysicsSystem = nullptr; }

    private:

        virtual bool LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override final;
        virtual void UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const override final;

    private:

        PhysicsSystem* m_pPhysicsSystem = nullptr;
    };
}