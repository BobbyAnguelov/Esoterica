#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Physics/PhysicsMaterial.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    struct EE_ENGINETOOLS_API PhysicsMaterialDatabaseResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( PhysicsMaterialDatabaseResourceDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return PhysicsMaterialDatabase::GetStaticResourceTypeID(); }

    public:

        EE_EXPOSE TVector<ResourcePath>         m_materialLibraries;
    };
}