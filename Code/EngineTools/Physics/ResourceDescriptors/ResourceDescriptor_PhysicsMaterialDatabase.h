#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Physics/PhysicsMaterial.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    struct EE_ENGINETOOLS_API PhysicsMaterialDatabaseResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( PhysicsMaterialDatabaseResourceDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return MaterialDatabase::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourcePath>& outDependencies ) override
        {
            for( auto const& matLibPath : m_materialLibraries )
            {
                if ( matLibPath.IsValid() )
                {
                    outDependencies.emplace_back( matLibPath );
                }
            }
        }

        virtual void Clear() override
        {
            m_materialLibraries.clear();
        }

    public:

        EE_REFLECT();
        TVector<ResourcePath>         m_materialLibraries;
    };
}