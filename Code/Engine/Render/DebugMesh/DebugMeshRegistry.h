#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderProxies.h"
#include "DebugMesh.h"
#include "Base/Systems.h"
#include "Base/Types/HashMap.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    namespace RHI
    {
        struct Buffer;
    }

    class RenderSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DebugMeshRegistry : public ISystem
    {
    public:
        struct RegisteredMesh
        {
            RegisteredMesh( uint64_t meshID, char const* pName, DebugMesh const& mesh )
                : m_meshID( meshID )
                , m_name( pName )
                , m_mesh( mesh )
            {}

            RegisteredMesh( uint64_t meshID, char const* pName, DebugMesh&& mesh )
                : m_meshID( meshID )
                , m_name( pName )
            {
                m_mesh.Swap( mesh );
            }

        public:

            uint64_t            m_meshID;
            String              m_name;
            DebugMesh           m_mesh;

            MeshHandle          m_meshHandle = {};
            ClustersHandle      m_clustersHandle = {};

            RHI::Buffer*        m_pClusterVertexBuffer = nullptr;
            RHI::Buffer*        m_pClusterTriangleBuffer = nullptr;
        };

    public:

        EE_SYSTEM( DebugMeshRegistry );

    public:

        DebugMeshRegistry() = default;
        ~DebugMeshRegistry();

        void Initialize( RenderSystem* pRenderSystem );
        void Shutdown();

        uint64_t RegisterMesh( InlineString const& name, DebugMesh const& mesh );
        uint64_t RegisterMesh( InlineString const& name, DebugMesh&& mesh );
        void UnregisterMesh( uint64_t meshID );

        bool IsValidMeshID( uint64_t meshID ) const 
        {
            EE_ASSERT( meshID > 0 );
            return m_registeredMeshes.find( meshID ) != m_registeredMeshes.end(); 
        }

        inline RegisteredMesh const* FindMesh( uint64_t meshID ) const
        {
            auto itr = m_registeredMeshes.find( meshID );
            if ( itr != m_registeredMeshes.end() )
            {
                return &itr->second;
            }
            return nullptr;
        } 

        DebugMesh const* GetDebugMesh( uint64_t meshID ) const
        {
            auto iter = m_registeredMeshes.find( meshID );
            if ( iter != m_registeredMeshes.end() )
            {
                return &iter->second.m_mesh;
            }

            return nullptr;
        }

        void UpdateDeviceResources();

    private:

        void CreateRenderMesh( RegisteredMesh& registeredMesh );
        void DestroyRenderMesh( RegisteredMesh& registeredMesh );

    private:

        mutable Threading::ReadWriteMutex   m_mutex;
        RenderSystem*                       m_pRenderSystem = nullptr;
        THashMap<uint64_t, RegisteredMesh>  m_registeredMeshes;
    };
}
#endif