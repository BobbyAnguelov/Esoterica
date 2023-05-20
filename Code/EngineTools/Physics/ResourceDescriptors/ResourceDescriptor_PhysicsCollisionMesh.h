#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    struct EE_ENGINETOOLS_API PhysicsCollisionMeshResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( PhysicsCollisionMeshResourceDescriptor );

    public:

        virtual bool IsValid() const override { return m_sourcePath.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return CollisionMesh::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if ( m_sourcePath.IsValid() )
            {
                outDependencies.emplace_back( m_sourcePath );
            }
        }

    public:

        EE_REFLECT() ResourcePath       m_sourcePath;

        // Optional: Specifies a single sub-mesh/sub-item to use to generated the physics collision, if this is not set, all sub-items contained in the source will be combined into a single collision setup
        EE_REFLECT() String             m_sourceItemName;

        // The default collision settings
        EE_REFLECT() CollisionSettings  m_collisionSettings;

        // Optional: Specifies if the collision setup is a convex shape
        EE_REFLECT() bool               m_isConvexMesh = false;

        // This allows you to perform non-uniform scaling/mirroring at import time since the engine does not support non-uniform scaling
        EE_REFLECT() Float3             m_scale = Float3( 1.0f, 1.0f, 1.0f );
    };
}