#pragma once
#include "EngineTools/MapEditor/MapEditorMode.h"
#include "EngineTools/Navmesh/NavmeshBuilder.h"
#include "EngineTools/Navmesh/NavmeshBuildData.h"
#include "Engine/Navmesh/NavmeshPath.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Render { class SkeletalMeshComponent; }
namespace EE::Animation { class AnimationClipPlayerComponent; }

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshWorldSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API NavigationMapEditorMode final : public EntityModel::MapEditorMode
    {
        EE_REFLECT_TYPE( NavigationMapEditorMode );

        enum class GenerationStage
        {
            None,
            ExtractBuildData,
            Generate,
            Report,
        };

    public:

        NavigationMapEditorMode() = default;
        ~NavigationMapEditorMode();

        virtual char const* GetName() const override { return "Navigation"; }
        virtual void UpdateAndDraw( UpdateContext const& context, bool isFocused ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Viewport const* pViewport, bool isViewportHovered, bool isViewportFocused ) override;
        virtual void Initialize( EntityModel::EditorContext* pEntityEditorContext ) override;
        virtual void Shutdown() override;

    private:

        void DrawNavmeshTab( UpdateContext const& context, bool isFocused );
        void DrawTesterTab( UpdateContext const& context, bool isFocused );

        virtual void PrePropertyGridChange( PropertyEditInfo const& info ) override;
        virtual void PostPropertyGridChange( PropertyEditInfo const& info ) override;

        ResourceID GetNavmeshResourceIDForCurrentlyEditedMap() const;
        FileSystem::Path GetUserGeneratedNavmeshFilePathForCurrentlyEditedMap() const;
        void ReloadNavmeshResourceForCurrentlyEditedMap();

        // Navmesh generation
        //-------------------------------------------------------------------------

        bool IsGeneratingNavmesh() const { return m_generationStage != GenerationStage::None; }
        void UpdateAndDrawExtractBuildDataStage( UpdateContext const& context );
        void UpdateAndDrawGenerateNavmeshStage( UpdateContext const& context );
        void DrawGenerationReportStage( UpdateContext const& context );

        // Tester
        //-------------------------------------------------------------------------

        void InitTester();
        void ShutdownTester();
        void SaveTestSettingsToIni();
        void LoadTestSettingsFromIni();

    private:

        Entity*                                             m_pNavmeshEntity = nullptr;
        NavmeshComponent*                                   m_pNavmeshComponent = nullptr;
        bool                                                m_hasMultipleNavmeshComponents = false;

        GenerationStage                                     m_generationStage = GenerationStage::None;
        NavmeshBuildData                                    m_buildData;
        EntityModel::EntityMapDescriptor                    m_mapDesc;
        
        ITaskSet*                                           m_pAsyncTask = nullptr;
        NavmeshData                                         m_navmeshData;

        //-------------------------------------------------------------------------

        NavmeshWorldSystem*                                 m_pWorldSystem;
        bool                                                m_isInTestMode = false;
        Transform                                           m_startTransform;
        Transform                                           m_endTransform;
        ImGuiX::Gizmo                                       m_startGizmo;
        ImGuiX::Gizmo                                       m_endGizmo;
        Entity*                                             m_pTestEntity = nullptr;
        Render::SkeletalMeshComponent*                      m_pTestMeshComponent = nullptr;
        Animation::AnimationClipPlayerComponent*            m_pTestAnimComponent = nullptr;
        Milliseconds                                        m_pathCalculationTime;
        bool                                                m_pathNeedsUpdate = false;

        #if EE_ENABLE_NAVPOWER
        Path                                                m_path;
        PathFollower                                        m_pathFollower;
        #endif

        //-------------------------------------------------------------------------

        #if EE_ENABLE_NAVPOWER
        NavmeshBuilder                                      m_builder;
        #endif
    };
}