#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Physics/PhysicsMesh.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    struct EE_ENGINETOOLS_API PhysicsMeshResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( PhysicsMeshResourceDescriptor );

    public:

        virtual bool IsValid() const override { return m_meshPath.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return PhysicsMesh::GetStaticResourceTypeID(); }

    public:

        EE_EXPOSE ResourcePath     m_meshPath;

        // Optional: Specifies a single sub-mesh to use to generated the physics collision, if this is not set, all sub-meshes contained in the source will be combined into a single mesh object
        EE_EXPOSE String           m_meshName;

        // Optional: Specifies if the mesh is a convex mesh (meshes are considered triangle meshes by default)
        EE_EXPOSE bool             m_isConvexMesh = false;
    };
}