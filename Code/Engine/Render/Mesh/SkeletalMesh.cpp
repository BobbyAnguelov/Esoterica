#include "SkeletalMesh.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    bool SkeletalMesh::IsValid() const
    {
        return m_indexBuffer.IsValid() && m_vertexBuffer.IsValid() && ( m_boneIDs.size() == m_parentBoneIndices.size() ) && ( m_boneIDs.size() == m_bindPose.size() );
    }

    int32_t SkeletalMesh::GetBoneIndex( StringID const& boneID ) const
    {
        auto const numBones = m_boneIDs.size();
        for ( auto i = 0; i < numBones; i++ )
        {
            if ( m_boneIDs[i] == boneID )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void SkeletalMesh::DrawBindPose( Drawing::DrawContext& drawingContext, Transform const& worldTransform ) const
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
        }
    }
    #endif
}