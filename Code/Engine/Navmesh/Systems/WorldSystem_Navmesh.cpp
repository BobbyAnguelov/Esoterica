#include "WorldSystem_Navmesh.h"
#include "Engine/Navmesh/NavPower.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Render/RenderViewport.h"
#include "Base/Profiling.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Drawing/DebugDrawingSystem.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    #if EE_ENABLE_NAVPOWER
    #if EE_DEVELOPMENT_TOOLS
    namespace Navpower
    {
        class Renderer final : public bfx::Renderer
        {

        public:

            void SetDebugDrawingSystem( Drawing::DrawingSystem* pDebugDrawingSystem )
            {
                m_pDebugDrawingSystem = pDebugDrawingSystem;
            }

            void Reset()
            {
                m_statsPos = Float2( 5, 20 );
            }

            inline bool IsDepthTestEnabled() const { return m_depthTestEnabled; }
            inline void SetDepthTestState( bool isEnabled ) { m_depthTestEnabled = isEnabled; }

        private:

            virtual void DrawLineList( bfx::LineSegment const* pLines, uint32_t numLines, bfx::Color const& color ) override
            {
                auto ctx = m_pDebugDrawingSystem->GetDrawingContext();
                for ( auto i = 0u; i < numLines; i++ )
                {
                    bfx::LineSegment const& line = pLines[i];
                    ctx.DrawLine( FromBfx( line.m_v0 ), FromBfx( line.m_v1 ), FromBfx( color ), 1.0f, m_depthTestEnabled ? Drawing::EnableDepthTest : Drawing::DisableDepthTest );
                }
            }

            virtual void DrawTriList( bfx::Triangle const* pTris, uint32_t numTris, bfx::Color const& color ) override
            {
                auto ctx = m_pDebugDrawingSystem->GetDrawingContext();
                for ( auto i = 0u; i < numTris; i++ )
                {
                    bfx::Triangle const& tri = pTris[i];
                    ctx.DrawTriangle( FromBfx( tri.m_v0 ), FromBfx( tri.m_v1 ), FromBfx( tri.m_v2 ), FromBfx( color ), m_depthTestEnabled ? Drawing::EnableDepthTest : Drawing::DisableDepthTest );
                }
            }

            virtual void DrawString( bfx::Color const& color, char const* str ) override
            {
                auto ctx = m_pDebugDrawingSystem->GetDrawingContext();
                ctx.DrawText2D( m_statsPos, str, FromBfx( color ), Drawing::FontSmall );
                m_statsPos += Float2( 0, 15 );
            }

            virtual void DrawString( bfx::Color const& color, bfx::Vector3 const& pos, char const* str ) override
            {
                auto ctx = m_pDebugDrawingSystem->GetDrawingContext();
                ctx.DrawText3D( FromBfx( pos ), str, FromBfx( color ), Drawing::FontSmall );
            }

        private:

            Drawing::DrawingSystem*                     m_pDebugDrawingSystem = nullptr;
            Float2                                      m_statsPos = Float2::Zero;
            bool                                        m_depthTestEnabled = true;
        };
    }
    #endif
    #endif

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    bool NavmeshWorldSystem::IsDebugRendererDepthTestEnabled() const
    {
        #if EE_ENABLE_NAVPOWER
        EE_ASSERT( m_pRenderer != nullptr );
        return m_pRenderer->IsDepthTestEnabled();
        #else
        return false;
        #endif
    }

    void NavmeshWorldSystem::SetDebugRendererDepthTestState( bool isDepthTestingEnabled )
    {
        #if EE_ENABLE_NAVPOWER
        EE_ASSERT( m_pRenderer != nullptr );
        m_pRenderer->SetDepthTestState( isDepthTestingEnabled );
        #endif
    }
    #endif

    //-------------------------------------------------------------------------

    void NavmeshWorldSystem::InitializeSystem( SystemRegistry const& systemRegistry )
    {
        #if EE_ENABLE_NAVPOWER
        m_pInstance = bfx::SystemCreate( bfx::SystemParams( 2.0f, bfx::Z_UP ), NavPower::GetAllocator() );
        bfx::SetCurrentInstance( nullptr );

        bfx::RegisterPlannerSystem( m_pInstance );
        bfx::SystemStart( m_pInstance );

        #if EE_DEVELOPMENT_TOOLS
        m_pRenderer = EE::New<Navpower::Renderer>();
        bfx::SetRenderer( m_pInstance, m_pRenderer );
        #endif
        #endif
    }

    void NavmeshWorldSystem::ShutdownSystem()
    {
        #if EE_ENABLE_NAVPOWER
        EE_ASSERT( m_registeredNavmeshes.empty() );

        #if EE_DEVELOPMENT_TOOLS
        bfx::SetRenderer( m_pInstance, nullptr );
        EE::Delete( m_pRenderer );
        #endif

        bfx::SystemStop( m_pInstance );
        bfx::SystemDestroy( m_pInstance );
        m_pInstance = nullptr;
        #endif
    }

    //-------------------------------------------------------------------------

    void NavmeshWorldSystem::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pNavmeshComponent = TryCast<NavmeshComponent>( pComponent ) )
        {
            m_navmeshComponents.emplace_back( pNavmeshComponent );

            if ( pNavmeshComponent->HasNavmeshData() )
            {
                RegisterNavmesh( pNavmeshComponent );
            }
        }
    }

    void NavmeshWorldSystem::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pNavmeshComponent = TryCast<NavmeshComponent>( pComponent ) )
        {
            if ( pNavmeshComponent->HasNavmeshData() )
            {
                UnregisterNavmesh( pNavmeshComponent );
            }

            m_navmeshComponents.erase_first_unsorted( pNavmeshComponent );
        }
    }

    void NavmeshWorldSystem::RegisterNavmesh( NavmeshComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );

        #if EE_ENABLE_NAVPOWER

        // Copy resource
        //-------------------------------------------------------------------------
        // NavPower operates on the resource in place so we need to make a copy

        NavmeshData const* pData = pComponent->m_pNavmeshData.GetPtr();
        EE_ASSERT( pData != nullptr && pData->IsValid() );

        size_t const requiredMemory = sizeof( char ) * pData->GetGraphImage().size();
        char* pNavmesh = (char*) EE::Alloc( requiredMemory );
        memcpy( pNavmesh, pData->GetGraphImage().data(), requiredMemory );

        // Add resource
        //-------------------------------------------------------------------------

        Transform const& componentWorldTransform = pComponent->GetWorldTransform();

        bfx::ResourceOffset offset;
        offset.m_positionOffset = ToBfx( componentWorldTransform.GetTranslation() );
        offset.m_rotationOffset = ToBfx( componentWorldTransform.GetRotation() );

        bfx::SpaceHandle space = bfx::GetDefaultSpaceHandle( m_pInstance );
        bfx::AddResource( space, pNavmesh, offset );

        // Add record
        m_registeredNavmeshes.emplace_back( RegisteredNavmesh( pComponent->GetID(), pNavmesh ) );

        #endif
    }

    void NavmeshWorldSystem::UnregisterNavmesh( NavmeshComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );
        #if EE_ENABLE_NAVPOWER

        for ( auto i = 0u; i < m_registeredNavmeshes.size(); i++ )
        {
            if ( pComponent->GetID() == m_registeredNavmeshes[i].m_componentID )
            {
                bfx::SpaceHandle space = bfx::GetDefaultSpaceHandle( m_pInstance );
                bfx::RemoveResource( space, m_registeredNavmeshes[i].m_pNavmesh );

                //-------------------------------------------------------------------------

                EE::Free( m_registeredNavmeshes[i].m_pNavmesh );
                m_registeredNavmeshes.erase_unsorted( m_registeredNavmeshes.begin() + i );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
        #endif
    }

    //-------------------------------------------------------------------------

    void NavmeshWorldSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        #if EE_ENABLE_NAVPOWER

        {
            EE_PROFILE_SCOPE_NAVIGATION( "Navmesh Simulate" );
            bfx::SystemSimulate( m_pInstance, ctx.GetDeltaTime() );
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        {
            EE_PROFILE_SCOPE_NAVIGATION( "Navmesh Debug Drawing" );
            m_pRenderer->SetDebugDrawingSystem( ctx.GetDebugDrawingSystem() );
            m_pRenderer->Reset();

            Render::Viewport const* pViewport = ctx.GetViewport();
        
            bfx::DrawCullParams cullParams;
            cullParams.m_cameraPos = ToBfx( pViewport->GetViewPosition() );
            cullParams.m_cameraDir = ToBfx( pViewport->GetViewForwardDirection() );
            cullParams.m_farClipDist = pViewport->GetViewVolume().GetDepthRange().m_end;
            cullParams.m_fov = pViewport->GetViewVolume().IsPerspective() ? pViewport->GetViewVolume().GetFOV().ToDegrees().ToFloat() : 0.0f;

            bfx::SystemDraw( m_pInstance, &cullParams );
        }
        #endif

        #endif
    }

    AABB NavmeshWorldSystem::GetNavmeshBounds( uint32_t layerIdx ) const
    {
        AABB bounds;

        #if EE_ENABLE_NAVPOWER
        bfx::SpaceHandle spaceHandle = bfx::GetDefaultSpaceHandle( m_pInstance );
        bfx::Vector3 center, extents;
        if ( bfx::GetNavGraphBounds( spaceHandle, 1 << layerIdx, center, extents ) )
        {
            bounds.m_center = FromBfx( center );
            bounds.m_halfExtents = FromBfx( extents );
        }
        #endif

        return bounds;
    }
}