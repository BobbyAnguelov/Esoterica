#pragma once
#include "EngineTools/Resource/RawFileInspector.h"
#include "EngineTools/RawAssets/Formats/FBX.h"
#include "System/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class RawFileInspectorFBX : public RawFileInspector
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
            bool                            m_isSkinned = false;
        };

        struct BoneSkeletonRootInfo
        {
            StringID                        m_nameID;
        };

        struct NullSkeletonRootInfo
        {
            StringID                        m_nameID;
            TVector<BoneSkeletonRootInfo>   m_childSkeletons;
        };

        struct AnimInfo
        {
            StringID                        m_nameID;
            Seconds                         m_duration = 0.0f;
            float                           m_frameRate = 0.0f;
        };

    public:

        RawFileInspectorFBX( ToolsContext const* pToolsContext, FileSystem::Path const& filePath );

    private:

        virtual char const* GetInspectorTitle() const override { return  "FBX Inspector"; }
        virtual void DrawFileInfo() override;
        virtual void DrawFileContents() override;
        virtual void DrawResourceDescriptorCreator() override;

        void ReadFileContents();

        void OnSwitchSelectedItem();

    private:

        Fbx::FbxSceneContext                m_sceneContext;
        TVector<MeshInfo>                   m_meshes;
        TVector<NullSkeletonRootInfo>       m_nullSkeletonRoots;
        TVector<BoneSkeletonRootInfo>       m_skeletonRoots;
        TVector<AnimInfo>                   m_animations;

        InfoType                            m_selectedItemType = InfoType::None;
        StringID                            m_selectedItemID;

    };
}