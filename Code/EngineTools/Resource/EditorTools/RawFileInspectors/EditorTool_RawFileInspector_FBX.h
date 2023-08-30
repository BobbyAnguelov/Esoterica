#pragma once
#include "EngineTools/Resource/EditorTools/EditorTool_RawFileInspector.h"
#include "EngineTools/RawAssets/Formats/FBX.h"
#include "EngineTools/Resource/ResourcePicker.h"

//-------------------------------------------------------------------------

namespace EE::Render { struct MeshResourceDescriptor; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API FbxFileInspectorEditorTool : public RawFileInspectorEditorTool
    {
        enum class InfoType
        {
            None,
            SkeletalMesh,
            StaticMesh,
            PhysicsCollision,
            Skeleton,
            Animation
        };

        struct MeshInfo
        {
            StringID                        m_nameID;
            StringID                        m_materialID;
            String                          m_extraInfo;
            bool                            m_isSkinned = false;
            bool                            m_isSelected = true;
        };

        struct SkeletonRootInfo
        {
            SkeletonRootInfo() = default;
            SkeletonRootInfo( StringID ID ) : m_nameID( ID ) {}

            inline bool IsNullOrLocatorNode() const { return !m_childSkeletonRoots.empty(); }
            inline bool IsSkeletonNode() const { return m_childSkeletonRoots.empty(); }

        public:

            StringID                        m_nameID;
            TVector<StringID>               m_childSkeletonRoots; // Only filled if this is a null root
        };

        struct AnimInfo
        {
            StringID                        m_nameID;
            Seconds                         m_duration = 0.0f;
            float                           m_frameRate = 0.0f;
        };

    public:

        FbxFileInspectorEditorTool( ToolsContext const* pToolsContext, FileSystem::Path const& rawFilePath );
        virtual void UpdateAndDrawInspectorWindow( UpdateContext const& context, bool isFocused ) override;

    private:

        void ReadFileContents();

        void DrawStaticMeshes();
        void DrawSkeletalMeshes();
        void DrawSkeletons();
        void DrawAnimations();

        void CreateStaticMeshDescriptor();
        void CreateSkeletalMeshDescriptor();
        void CreatePhysicsMeshDescriptor();
        void CreateSkeletonDescriptor( StringID skeletonID );
        void CreateAnimationDescriptor( StringID animTakeID );

        void SetupMaterialsForMeshDescriptor( FileSystem::Path const& destinationFolder, Render::MeshResourceDescriptor& meshDescriptor );

        inline bool AreAllMeshesSelected() const
        {
            for ( auto& meshInfo : m_meshes )
            {
                if ( !meshInfo.m_isSelected )
                {
                    return false;
                }
            }

            return true;
        }

    private:

        Fbx::SceneContext                m_sceneContext;
        TVector<MeshInfo>                   m_meshes;
        TVector<SkeletonRootInfo>           m_skeletonRoots;
        TVector<AnimInfo>                   m_animations;
        bool                                m_hasSkeletalMeshes = false;
        bool                                m_refreshFileInfo = false;

        // Descriptor Options
        bool                                m_setupMaterials = false;
        bool                                m_mergesMeshesWithSameMaterial = true;
        Float3                              m_meshScale = Float3( 1, 1, 1 );
        Resource::ResourcePicker            m_skeletonPicker;
    };
}