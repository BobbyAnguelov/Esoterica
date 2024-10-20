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
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return CollisionMesh::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return CollisionMesh::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return CollisionMesh::s_friendlyName; }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            if ( m_sourcePath.IsValid() )
            {
                outDependencies.emplace_back( m_sourcePath );
            }
        }

        virtual void Clear() override
        {
            m_sourcePath.Clear();
            m_meshesToInclude.clear();
            m_collisionSettings.Clear();
            m_isConvexMesh = false;
            m_scale = Float3( 1.0f, 1.0f, 1.0f );
        }

    public:

        EE_REFLECT();
        DataPath        m_sourcePath;

        // Optional: specifies the specific sub-meshes to compile, if this is not set, all sub-meshes contained in the source will be combined into a single mesh object
        EE_REFLECT();
        TVector<String>     m_meshesToInclude;

        // The default collision settings
        EE_REFLECT();
        CollisionSettings   m_collisionSettings;

        // Optional: Specifies if the collision setup is a convex shape
        EE_REFLECT();
        bool                m_isConvexMesh = false;

        // This allows you to perform non-uniform scaling/mirroring at import time since the engine does not support non-uniform scaling
        EE_REFLECT();
        Float3              m_scale = Float3( 1.0f, 1.0f, 1.0f );
    };
}