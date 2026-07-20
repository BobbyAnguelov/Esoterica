#include "RenderMesh.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    Mesh::Socket const* Mesh::GetSocket( StringID socketID ) const
    {
        for ( auto& socket : m_sockets )
        {
            if ( socket.m_ID == socketID )
            {
                return &socket;
            }
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    bool SkeletalMesh::IsValid() const
    {
        return Mesh::IsValid() && ( m_boneIDs.size() == m_parentBoneIndices.size() ) && ( m_boneIDs.size() == m_bindPose.size() );
    }

    int32_t SkeletalMesh::GetBoneIndex( StringID const& boneID ) const
    {
        size_t const numBones = m_boneIDs.size();
        for ( size_t i = 0; i < numBones; i++ )
        {
            if ( m_boneIDs[i] == boneID )
            {
                return (int32_t) i;
            }
        }

        return InvalidIndex;
    }

    bool SkeletalMesh::IsChildBoneOf( int32_t parentBoneIdx, int32_t childBoneIdx ) const
    {
        EE_ASSERT( IsValidBoneIndex( parentBoneIdx ) );
        EE_ASSERT( IsValidBoneIndex( childBoneIdx ) );

        bool isChild = false;

        int32_t actualParentBoneIdx = GetParentBoneIndex( childBoneIdx );
        while ( actualParentBoneIdx != InvalidIndex )
        {
            if ( actualParentBoneIdx == parentBoneIdx )
            {
                isChild = true;
                break;
            }

            actualParentBoneIdx = GetParentBoneIndex( actualParentBoneIdx );
        }

        return isChild;
    }

    #if EE_DEVELOPMENT_TOOLS
    void SkeletalMesh::DrawBindPose( DebugDrawContext& drawingContext, Transform const& worldTransform, bool drawBoneNames ) const
    {
        auto const numBones = GetNumBones();

        Transform boneWorldTransform = m_bindPose[0] * worldTransform;
        drawingContext.DrawBox( boneWorldTransform, Float3( 0.005f ), Colors::Orange );
        drawingContext.DrawAxis( boneWorldTransform, 0.05f );

        for ( auto i = 1; i < numBones; i++ )
        {
            boneWorldTransform = m_bindPose[i] * worldTransform;

            auto const parentBoneIdx = GetParentBoneIndex( i );
            Transform const parentBoneWorldTransform = m_bindPose[parentBoneIdx] * worldTransform;

            drawingContext.DrawLine( parentBoneWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), Colors::Orange );
            drawingContext.DrawBox( boneWorldTransform, Float3( 0.005f ), Colors::Orange );
            drawingContext.DrawAxis( boneWorldTransform, 0.05f, 2.0f );

            if ( drawBoneNames )
            {
                drawingContext.DrawText3D( boneWorldTransform.GetTranslation(), GetBoneID( i ).c_str(), Colors::Orange, DebugFont::Small );
            }
        }
    }
    #endif
}
