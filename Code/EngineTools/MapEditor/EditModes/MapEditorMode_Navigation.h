#pragma once
#include "EngineTools/MapEditor/MapEditorMode.h"

#include "EngineTools/Navmesh/NavmeshBuilder.h"
#include "EngineTools/Navmesh/NavmeshBuildData.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Threading/TaskSystem.h"


//-------------------------------------------------------------------------

namespace EE::Navmesh
{
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

    private:


        Entity*                                             m_pNavmeshEntity = nullptr;
        NavmeshComponent*                                   m_pNavmeshComponent = nullptr;
        bool                                                m_hasMultipleNavmeshComponents = false;

        GenerationStage                                     m_generationStage = GenerationStage::None;
        NavmeshBuildData                                    m_buildData;
        EntityModel::EntityMapDescriptor                    m_mapDesc;
        
        ITaskSet*                                           m_pAsyncTask = nullptr;
        NavmeshData                                         m_navmeshData;

        #if EE_ENABLE_NAVPOWER
        NavmeshBuilder                                      m_builder;
        #endif
    };
}