#pragma once
#if EE_ENABLE_NAVPOWER

#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/DataPath.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Math/Transform.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/Types/HashMap.h"
#include <bfxBuilder.h>

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }
namespace EE::EntityModel { class EntityCollection; }

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    struct NavmeshBuildSettings;
    class NavmeshData;

    //-------------------------------------------------------------------------

    class NavmeshGenerator : public bfx::BuildProgressMonitor
    {
    public:

        enum class State
        {
            Idle,
            Generating,
            CompletedSuccess,
            CompletedFailure
        };

        struct CollisionMesh
        {
            Transform   m_worldTransform;
            Vector      m_localScale;
        };

    public:

        NavmeshGenerator( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& outputPath, EntityModel::EntityCollection const& entityCollection, NavmeshBuildSettings const& buildSettings );
        ~NavmeshGenerator();

        inline char const* GetProgressMessage() const { return m_progressMessage; }
        inline float GetProgressBarValue() const { return m_progress; }
        inline State GetState() const { return m_state; }

        // Kick off the async generation task
        void GenerateAsync( TaskSystem& taskSystem );

        // Generates the navmesh via a blocking call - returns true if the generation succeeded, false otherwise
        bool GenerateSync() { Generate(); return m_state == State::CompletedSuccess; }

    private:

        virtual void BuildProgressUpdate( float percentDone ) override { m_progress = percentDone / 100.0f; }

        //-------------------------------------------------------------------------

        void Generate();

        //-------------------------------------------------------------------------

        bool CollectCollisionPrimitives();

        bool CollectTriangles();

        bool BuildNavmesh( NavmeshData& navmeshData );

        bool SaveNavmesh( NavmeshData& navmeshData );

    private:

        // Build data
        FileSystem::Path const                          m_rawResourceDirectoryPath;
        FileSystem::Path const                          m_outputPath;
        TypeSystem::TypeRegistry const&                 m_typeRegistry;
        EntityModel::EntityCollection const&            m_entityCollection;
        NavmeshBuildSettings const&                     m_buildSettings;
        
        // Build transient data
        bfx::Instance*                                  m_pNavpowerInstance = nullptr;
        THashMap<DataPath, TVector<CollisionMesh>>      m_collisionPrimitives;
        size_t                                          m_numCollisionPrimitivesToProcess = 0;
        TVector<bfx::BuildFace>                         m_buildFaces;

        // Generator state
        char                                            m_progressMessage[256];
        float                                           m_progress = 0.0f;
        State                                           m_state = State::Idle;
        AsyncTask                                       m_asyncTask;
        bool                                            m_isGeneratingAsync = false;
    };
}

#endif