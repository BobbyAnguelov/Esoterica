#pragma once

#include "Engine/_Module/API.h"
#include "System/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class MaterialRegistry;

    //-------------------------------------------------------------------------

    class PhysicsMaterialDatabaseLoader : public Resource::ResourceLoader
    {
    public:

        PhysicsMaterialDatabaseLoader();

        void SetMaterialRegistryPtr( MaterialRegistry* pRegistry )
        {
            EE_ASSERT( pRegistry != nullptr );
            m_pRegistry = pRegistry; 
        }

        void ClearMaterialRegistryPtr() { m_pRegistry = nullptr; }

    private:

        virtual bool LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override final;
        virtual void UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const override final;

    private:

        MaterialRegistry* m_pRegistry = nullptr;
    };
}