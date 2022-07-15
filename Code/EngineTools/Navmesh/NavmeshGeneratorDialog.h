#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Engine/Entity/EntityDescriptors.h"

//-------------------------------------------------------------------------

namespace EE { class ToolsContext; class UpdateContext; }
namespace EE::TypeSystem { class TypeRegistry; }
namespace EE::EntityModel { class EntityCollectionDescriptor; }

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshGenerator;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API NavmeshGeneratorDialog
    {

    public:

        NavmeshGeneratorDialog( ToolsContext const* pToolsContext, NavmeshBuildSettings const& initialBuildSettings, EntityModel::EntityCollectionDescriptor const& entityCollectionDesc, FileSystem::Path const& navmeshOutputPath );
        ~NavmeshGeneratorDialog();

        bool UpdateAndDrawDialog( UpdateContext const& ctx );

        NavmeshBuildSettings const& GetBuildSettings() const { return m_buildSettings; }
        bool WereBuildSettingsUpdated() const { return m_propertyGrid.IsDirty(); }

    private:

        ToolsContext const*                                 m_pToolsContext = nullptr;
        NavmeshBuildSettings                                m_buildSettings;
        EntityModel::EntityCollectionDescriptor const       m_entityCollectionDesc;
        FileSystem::Path const                              m_navmeshOutputPath;
        PropertyGrid                                        m_propertyGrid;
        NavmeshGenerator*                                   m_pGenerator = nullptr;
    };
}