#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Navmesh/NavmeshData.h"
#include "Base/Resource/ResourcePtr.h"
#include "Engine/Entity/EntitySpatialComponent.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    struct EE_ENGINE_API NavmeshLayerBuildSettings : public IReflectedType
    {
        EE_REFLECT_TYPE( NavmeshLayerBuildSettings );

        EE_REFLECT() StringID                                 m_layerID; // User supplied layer ID to help identify layers, not actually used
        EE_REFLECT() float                                    m_voxSize = 0.1f; // Size of auto-generation voxels
        EE_REFLECT() float                                    m_radius = 0.45f; // Body radius of the creatures which will use this NavGraph.  The NavGraph will be retracted by approximately this radius away from walls.
        EE_REFLECT() float                                    m_dropOffRadius = 0.2f;// The distance that the NavGraph should be retracted from dropOffs (a.k.a. ledges or falling edges).  This value can be slightly smaller than the m_radius.
        EE_REFLECT() float                                    m_step = 0.2f; // Maximum step height for surface movement
        EE_REFLECT() float                                    m_height = 2.0f; // Clearance height for the creatures which will use this NavGraph
        EE_REFLECT() int                                      m_maxNumIslands = 0; // The build process will enforce this optional limit by removing small islands. A value of 0 disables the feature, i.e. no islands are removed.
        EE_REFLECT() float                                    m_minIslandSurfaceArea = 2.0f; // The build process will eliminate any islands with a surface area smaller than this limit (distance units squared)
        EE_REFLECT() bool                                     m_leaveSmallIslandsTouchingPortals = true; // If true then the build process will ignore m_maxNumIslands and m_minIslandSurfaceArea for islands touching a streaming portal.
        EE_REFLECT() float                                    m_additionalInwardsSmoothingDist = 0.0f; // The build process will perform additional smoothing on the NavGraph edges, moving vertices inwards by up to this distance.
        EE_REFLECT() float                                    m_maxWalkableSlope = 40.0f;	// The maximum slope (degrees) for a BuildFace of type WALKABLE_FACE.  Faces steeper than this will be treated as being of type OBSTACLE_FACE.
        EE_REFLECT() float                                    m_verticalOffsetDist = 0.1f; // The auto-gen NavGraph will be offset vertically by this additional distance.  Positive values move the NavGraph upwards.
        EE_REFLECT() bool                                     m_useEnhancedTerrainTracking = true; // The auto-gen NavGraph will track the terrain more closely, especially in hilly areas.
        EE_REFLECT() bool                                     m_optimizeForAxisAligned = false; // Optimizes the NavGraph assuming that the level contains mostly axis aligned walls, obstacles, etc.
        EE_REFLECT() bool                                     m_tessellateForPathingAccuracy = false; // Tessellate the NavGraph for improved accuracy at runtime at the cost of a slight run-time performance penalty.
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API NavmeshBuildSettings : public IReflectedType
    {
        EE_REFLECT_TYPE( NavmeshBuildSettings );

        EE_REFLECT() NavmeshLayerBuildSettings                m_defaultLayerBuildSettings;
        EE_REFLECT() TVector<NavmeshLayerBuildSettings>       m_additionalLayerBuildSettings;
        EE_REFLECT() bool                                     m_enableBuildLogging = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API NavmeshComponent : public SpatialEntityComponent
    {
        EE_SINGLETON_ENTITY_COMPONENT( NavmeshComponent );
        friend class NavmeshWorldSystem;

    public:

        using SpatialEntityComponent::SpatialEntityComponent;
        NavmeshComponent( ResourceID const& navmeshResourceID ) : m_navmeshData( navmeshResourceID ) {}

        inline bool HasNavmeshData() const { return m_navmeshData.IsLoaded() && m_navmeshData->IsValid(); }

        #if EE_DEVELOPMENT_TOOLS
        NavmeshBuildSettings const& GetBuildSettings() const { return m_buildSettings; }
        void SetBuildSettings( NavmeshBuildSettings const& buildSettings ) { m_buildSettings = buildSettings; }
        #endif

    private:

        #if EE_DEVELOPMENT_TOOLS
        EE_REFLECT() 
        NavmeshBuildSettings                    m_buildSettings;
        #endif

        EE_REFLECT( ReadOnly )
        TResourcePtr<NavmeshData>               m_navmeshData;
    };
}