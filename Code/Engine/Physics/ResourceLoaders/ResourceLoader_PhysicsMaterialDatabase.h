#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourceLoader.h"

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

        virtual bool Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const override final;
        virtual void UnloadInternal( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override final;

    private:

        MaterialRegistry* m_pRegistry = nullptr;
    };
}