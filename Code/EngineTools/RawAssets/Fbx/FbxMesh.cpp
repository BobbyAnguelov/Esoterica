#include "FbxMesh.h"
#include "FbxSceneContext.h"
#include "FbxSkeleton.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace RawAssets
    {
        class FbxRawSkeleton : public RawSkeleton
        {
            friend class FbxMeshFileReader;
        };

        class FbxRawMesh : public RawMesh
        {
            friend class FbxMeshFileReader;
        };

        //-------------------------------------------------------------------------

        class FbxMeshFileReader
        {
        public:

            template<typename ElementType, typename ElementDataType>
            static ElementDataType GetElementData( ElementType* pElement, int32_t const ctrlPointIdx, int32_t const vertexIdx )
            {
                ElementDataType data;
                switch ( pElement->GetMappingMode() )
                {
                    case FbxGeometryElement::eByControlPoint:
                    {
                        switch ( pElement->GetReferenceMode() )
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                data = pElement->GetDirectArray().GetAt( ctrlPointIdx );
                            }
                            break;

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                int32_t const dataElementIdx = pElement->GetIndexArray().GetAt( ctrlPointIdx );
                                data = pElement->GetDirectArray().GetAt( dataElementIdx );
                            }
                            break;

                            default:
                            break;
                        }
                    }
                    break;

                    case FbxGeometryElement::eByPolygonVertex:
                    {
                        switch ( pElement->GetReferenceMode() )
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                data = pElement->GetDirectArray().GetAt( vertexIdx );
                            }
                            break;

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                int32_t const dataElementIdx = pElement->GetIndexArray().GetAt( vertexIdx );
                                data = pElement->GetDirectArray().GetAt( dataElementIdx );
                            }
                            break;

                            default:
                            break;
                        }
                    }
                    break;

                    default:
                    break;
                }

                return data;
            }

            //-------------------------------------------------------------------------

            static TUniquePtr<RawMesh> ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude )
            {
                EE_ASSERT( sourceFilePath.IsValid() );

                TUniquePtr<RawMesh> pMesh( EE::New<RawMesh>() );
                FbxRawMesh* pRawMesh = (FbxRawMesh*) pMesh.get();

                //-------------------------------------------------------------------------

                Fbx::FbxSceneContext sceneCtx( sourceFilePath );
                if ( sceneCtx.IsValid() )
                {
                    ReadAllSubmeshes( *pRawMesh, sceneCtx, meshesToInclude );
                }
                else
                {
                    pRawMesh->LogError( "Failed to read FBX file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
                }

                return pMesh;
            }

            static TUniquePtr<RawMesh> ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences = 4 )
            {
                EE_ASSERT( sourceFilePath.IsValid() );

                TUniquePtr<RawMesh> pMesh( EE::New<RawMesh>() );
                FbxRawMesh* pRawMesh = (FbxRawMesh*) pMesh.get();

                //-------------------------------------------------------------------------

                Fbx::FbxSceneContext sceneCtx( sourceFilePath );
                if ( sceneCtx.IsValid() )
                {
                    pRawMesh->m_maxNumberOfBoneInfluences = maxBoneInfluences;
                    pRawMesh->m_isSkeletalMesh = true;

                    // Read mesh data
                    //-------------------------------------------------------------------------

                    ReadAllSubmeshes( *pRawMesh, sceneCtx, meshesToInclude );

                    if ( pRawMesh->HasErrors() )
                    {
                        return pMesh;
                    }

                    // Since the bind pose is in global space, calculate the local space transforms for the skeleton
                    FbxRawSkeleton& rawSkeleton = (FbxRawSkeleton&) pRawMesh->m_skeleton;
                    rawSkeleton.CalculateLocalTransforms();
                }
                else
                {
                    pRawMesh->LogError( "Failed to read FBX file: %s -> %s", sourceFilePath.c_str(), sceneCtx.GetErrorMessage().c_str() );
                }

                return pMesh;
            }

            //-------------------------------------------------------------------------

            static void ReadAllSubmeshes( FbxRawMesh& rawMesh, Fbx::FbxSceneContext const& sceneCtx, TVector<String> const& meshesToInclude )
            {
                FbxGeometryConverter geomConverter( sceneCtx.m_pManager );
                geomConverter.SplitMeshesPerMaterial( sceneCtx.m_pScene, true );

                // Find all meshes in the sceneCtx
                //-------------------------------------------------------------------------

                struct FoundMesh
                {
                    FbxMesh* m_pMesh ;
                    bool isSkinned;
                };

                TInlineVector<FoundMesh, 20> meshes;
                auto numGeometries = sceneCtx.m_pScene->GetGeometryCount();

                if ( numGeometries == 0 )
                {
                    rawMesh.m_errors.push_back( "No meshes present in the sceneCtx" );
                    return;
                }

                for ( auto i = 0; i < numGeometries; i++ )
                {
                    auto pGeometry = sceneCtx.m_pScene->GetGeometry( i );
                    if ( pGeometry->Is<FbxMesh>() )
                    {
                        FbxMesh* pMesh = static_cast<FbxMesh*>( pGeometry );
                        meshes.push_back( { pMesh, pMesh->GetDeformerCount(FbxDeformer::eSkin) > 0 } );
                    }
                }

                // For each mesh found perform necessary corrections and read mesh data
                // Note: this needs to be done in two passes since these operations reorder the geometries in the sceneCtx and pScene->GetGeometry( x ) doesnt return what you expect
                bool meshFound = false;
                TVector<TVector<uint32_t>> controlPointVertexMapping;
                for ( auto foundMesh : meshes )
                {
                    if ( rawMesh.m_isSkeletalMesh && !foundMesh.isSkinned )
                    {
                        continue;
                    }

                    auto pMesh = foundMesh.m_pMesh;

                    // Only process specified meshes
                    if ( !meshesToInclude.empty() )
                    {
                        if ( !VectorContains( meshesToInclude, pMesh->GetNode()->GetNameWithoutNameSpacePrefix() ) )
                        {
                            continue;
                        }
                    }

                    //-------------------------------------------------------------------------

                    // Prepare the mesh for reading
                    meshFound = true;
                    if ( !PrepareMesh( sceneCtx, geomConverter, pMesh, rawMesh ) )
                    {
                        return;
                    }

                    // Allocate space for the control point mapping
                    controlPointVertexMapping.clear();
                    controlPointVertexMapping.resize( pMesh->GetControlPointsCount() );

                    // Create new geometry section
                    RawMesh::GeometrySection& meshData = rawMesh.m_geometrySections.emplace_back( RawMesh::GeometrySection() );
                    meshData.m_name = (char const*) pMesh->GetNode()->GetNameWithNameSpacePrefix();
                    meshData.m_numUVChannels = pMesh->GetElementUVCount();

                    if ( !ReadMeshData( sceneCtx, pMesh, meshData, controlPointVertexMapping ) )
                    {
                        return;
                    }

                    if ( rawMesh.m_isSkeletalMesh )
                    {
                        if ( !ReadSkinningData( rawMesh, sceneCtx, pMesh, meshData, controlPointVertexMapping ) )
                        {
                            return;
                        }
                    }
                }

                //-------------------------------------------------------------------------

                if ( !meshFound )
                {
                    InlineString meshNames;
                    for ( auto const& meshName : meshesToInclude )
                    {
                        meshNames.append_sprintf( "%s, ", meshName.c_str() );
                    }
                    meshNames = meshNames.substr( 0, meshNames.length() - 2 );

                    rawMesh.m_errors.push_back( String( String::CtorSprintf(), "Couldn't find any specified meshes: %s", meshNames.c_str() ) );
                }
            }

            //-------------------------------------------------------------------------

            static bool PrepareMesh( Fbx::FbxSceneContext const& sceneCtx, FbxGeometryConverter& geomConverter, fbxsdk::FbxMesh*& pMesh, FbxRawMesh& rawMesh )
            {
                pMesh->RemoveBadPolygons();

                // Ensure that the mesh is triangulated
                if ( !pMesh->IsTriangleMesh() )
                {
                    pMesh = (fbxsdk::FbxMesh*) geomConverter.Triangulate( pMesh, true );
                    EE_ASSERT( pMesh != nullptr && pMesh->IsTriangleMesh() );
                }

                // Generate normals if they're not available or not in the right format
                bool const hasNormals = pMesh->GetElementNormalCount() > 0;
                bool const hasPerVertexNormals = hasNormals && pMesh->GetElementNormal()->GetMappingMode() == FbxLayerElement::eByPolygonVertex;
                if ( !hasPerVertexNormals )
                {
                    if ( !pMesh->GenerateNormals( true, false, false ) )
                    {
                        rawMesh.m_errors.push_back( String( String::CtorSprintf(), "Failed to regenerate mesh normals for mesh: %s", (char const*) pMesh->GetNameWithNameSpacePrefix() ) );
                        return false;
                    }
                }

                // Generate smoothing groups if they doesnt exist
                if ( pMesh->GetElementSmoothingCount() == 0 )
                {
                    geomConverter.ComputeEdgeSmoothingFromNormals( pMesh );
                    geomConverter.ComputePolygonSmoothingFromEdgeSmoothing( pMesh );
                }

                EE_ASSERT( pMesh->GetElementNormal()->GetMappingMode() == FbxLayerElement::eByPolygonVertex );
                return true;
            }

            static bool ReadMeshData( Fbx::FbxSceneContext const& sceneCtx, fbxsdk::FbxMesh* pMesh, FbxRawMesh::GeometrySection& geometryData, TVector<TVector<uint32_t>>& controlPointVertexMapping )
            {
                EE_ASSERT( pMesh != nullptr && pMesh->IsTriangleMesh() );

                FbxAMatrix const meshNodeGlobalTransform = sceneCtx.GetNodeGlobalTransform( pMesh->GetNode() );

                //-------------------------------------------------------------------------

                // Check winding order - TODO: actually figure out how to get the real value, right now we assume CCW
                geometryData.m_clockwiseWinding = false;
                FbxVector4 const meshScale = meshNodeGlobalTransform.GetS();
                uint32_t const numNegativeAxes = ( ( meshScale[0] < 0 ) ? 1 : 0 ) + ( ( meshScale[1] < 0 ) ? 1 : 0 ) + ( ( meshScale[2] < 0 ) ? 1 : 0 );
                if ( numNegativeAxes == 1 || numNegativeAxes == 3 )
                {
                    geometryData.m_clockwiseWinding = !geometryData.m_clockwiseWinding;
                }

                // Reserve memory for mesh data
                int32_t const numPolygons = pMesh->GetPolygonCount();
                int32_t const numVertices = numPolygons * 3;
                geometryData.m_vertices.reserve( numVertices );

                for ( int32_t polygonIdx = 0; polygonIdx < numPolygons; polygonIdx++ )
                {
                    for ( int32_t vertexIdx = 0; vertexIdx < 3; vertexIdx++ )
                    {
                        RawMesh::VertexData vert;

                        // Get vertex position
                        //-------------------------------------------------------------------------

                        int32_t const ctrlPointIdx = pMesh->GetPolygonVertex( polygonIdx, vertexIdx );
                        FbxVector4 const meshVertex = meshNodeGlobalTransform.MultT( pMesh->GetControlPoints()[ctrlPointIdx] );
                        vert.m_position = sceneCtx.ConvertVector3AndFixScale( meshVertex );
                        vert.m_position.m_w = 1.0f;

                        // Get vertex color
                        //-------------------------------------------------------------------------

                        FbxLayerElementVertexColor* pColorElement = pMesh->GetElementVertexColor();
                        if ( pColorElement != nullptr )
                        {
                            FbxColor const color = GetElementData<FbxLayerElementVertexColor, FbxColor>( pColorElement, ctrlPointIdx, vertexIdx );
                            vert.m_color = Float4( (float) color.mRed, (float) color.mGreen, (float) color.mBlue, (float) color.mAlpha );
                        }

                        // Get vertex normal
                        //-------------------------------------------------------------------------

                        EE_ASSERT( pMesh->GetElementNormal() != nullptr );
                        FbxVector4 meshNormal;
                        pMesh->GetPolygonVertexNormal( polygonIdx, vertexIdx, meshNormal );
                        vert.m_normal = sceneCtx.ConvertVector3( meshNormal ).GetNormalized3();

                        // Get vertex tangent and bi-normals
                        //-------------------------------------------------------------------------

                        FbxGeometryElementTangent* pTangentElement = pMesh->GetElementTangent();
                        if ( pTangentElement != nullptr )
                        {
                            FbxVector4 const tangent = GetElementData<FbxGeometryElementTangent, FbxVector4>( pTangentElement, ctrlPointIdx, vertexIdx );
                            vert.m_tangent = sceneCtx.ConvertVector3( tangent ).GetNormalized3();
                        }

                        FbxGeometryElementBinormal* pBinormalElement = pMesh->GetElementBinormal();
                        if ( pBinormalElement != nullptr )
                        {
                            FbxVector4 const binormal = GetElementData<FbxGeometryElementBinormal, FbxVector4>( pBinormalElement, ctrlPointIdx, vertexIdx );
                            vert.m_binormal = sceneCtx.ConvertVector3( binormal ).GetNormalized3();
                        }

                        // Get vertex UV
                        //-------------------------------------------------------------------------

                        int32_t const numUVChannelsForMeshSection = pMesh->GetElementUVCount();
                        for ( auto i = 0; i < numUVChannelsForMeshSection; ++i )
                        {
                            FbxGeometryElementUV* pTexcoordElement = pMesh->GetElementUV( i );
                            FbxVector2 texCoord( 0, 0 );

                            switch ( pTexcoordElement->GetMappingMode() )
                            {
                                case FbxGeometryElement::eByControlPoint:
                                {
                                    switch ( pTexcoordElement->GetReferenceMode() )
                                    {
                                        case FbxLayerElementUV::eDirect:
                                        {
                                            texCoord = pTexcoordElement->GetDirectArray().GetAt( ctrlPointIdx );
                                        }
                                        break;

                                        case FbxLayerElementUV::eIndexToDirect:
                                        {
                                            int32_t const texCoordIdx = pTexcoordElement->GetIndexArray().GetAt( ctrlPointIdx );
                                            texCoord = pTexcoordElement->GetDirectArray().GetAt( texCoordIdx );
                                        }
                                        break;

                                        default:
                                        break;
                                    }
                                }
                                break;

                                // This is special for UVs
                                case FbxGeometryElement::eByPolygonVertex:
                                {
                                    switch ( pTexcoordElement->GetReferenceMode() )
                                    {
                                        case FbxLayerElementUV::eDirect:
                                        case FbxLayerElementUV::eIndexToDirect:
                                        {
                                            int32_t const textureUVIndex = pMesh->GetTextureUVIndex( polygonIdx, vertexIdx );
                                            texCoord = pTexcoordElement->GetDirectArray().GetAt( textureUVIndex );
                                        }
                                        break;

                                        default:
                                        break;
                                    }
                                }
                                break;

                                //-------------------------------------------------------------------------

                                default:
                                break;
                            }

                            vert.m_texCoords.push_back( Float2( (float) texCoord[0], 1.0f - (float) texCoord[1] ) );
                        }

                        // Ensure we always have UV's set
                        if ( vert.m_texCoords.empty() )
                        {
                            vert.m_texCoords.emplace_back( Float2::Zero );
                        }

                        // Add vertex to mesh data
                        //-------------------------------------------------------------------------

                        // Check whether we are referencing a new vertex or should use the index to an existing one?
                        uint32_t existingVertexIdx = (uint32_t) InvalidIndex;
                        auto& vertexIndices = controlPointVertexMapping[ctrlPointIdx];
                        for ( auto const& idx : vertexIndices )
                        {
                            if ( vert == geometryData.m_vertices[idx] )
                            {
                                existingVertexIdx = idx;
                                break;
                            }
                        }

                        // The vertex already exists, so just add its index
                        if ( existingVertexIdx != (uint32_t) InvalidIndex )
                        {
                            EE_ASSERT( existingVertexIdx < geometryData.m_vertices.size() );
                            geometryData.m_indices.push_back( existingVertexIdx );
                        }
                        else // Create new vertex
                        {
                            geometryData.m_indices.push_back( (uint32_t) geometryData.m_vertices.size() );
                            vertexIndices.push_back( (uint32_t) geometryData.m_vertices.size() );
                            geometryData.m_vertices.push_back( vert );
                        }
                    }
                }

                return true;
            }

            //-------------------------------------------------------------------------

            static FbxNode* FindSkeletonForSkin( FbxSkin* pSkin )
            {
                FbxNode* pSkeletonRootNode = nullptr;

                //-------------------------------------------------------------------------

                FbxCluster* pCluster = pSkin->GetCluster( 0 );
                FbxNode* pBoneNode = pCluster->GetLink();

                while ( pBoneNode != nullptr )
                {
                    // Traverse hierarchy to find the root of the skeleton
                    FbxNodeAttribute* pNodeAttribute = pBoneNode->GetNodeAttribute();
                    if ( pNodeAttribute != nullptr && pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton )
                    {
                        pSkeletonRootNode = pBoneNode;
                        pBoneNode = pBoneNode->GetParent();
                    }
                    else
                    {
                        pBoneNode = nullptr;
                    }
                }

                // If the root of the skeleton has a null root, make that the root as this is a common practice
                // TODO: should we make this a setting in the descriptor?
                if ( auto pParentNode = pSkeletonRootNode->GetParent() )
                {
                    if ( auto pNodeAttribute = pParentNode->GetNodeAttribute() )
                    {
                        if ( pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNull )
                        {
                            pSkeletonRootNode = pParentNode;
                        }
                    }
                }

                return pSkeletonRootNode;
            }

            static bool ReadSkinningData( FbxRawMesh& rawMesh, Fbx::FbxSceneContext const& sceneCtx, fbxsdk::FbxMesh* pMesh, RawMesh::GeometrySection& geometryData, TVector<TVector<uint32_t>>& controlPointVertexMapping )
            {
                EE_ASSERT( pMesh != nullptr && pMesh->IsTriangleMesh() && rawMesh.m_isSkeletalMesh );

                // Check whether skinning data exists for this mesh
                int32_t const numSkins = pMesh->GetDeformerCount( FbxDeformer::eSkin );
                if ( numSkins == 0 )
                {
                    rawMesh.LogError( "No skin data found!" );
                    return false;
                }

                if ( numSkins > 1 )
                {
                    rawMesh.LogWarning( "More than one skin found for mesh (%s), compiling only the first skin found!", geometryData.m_name.c_str() );
                }

                // Read skin
                //-------------------------------------------------------------------------

                FbxSkin* pSkin = static_cast<FbxSkin*>( pMesh->GetDeformer( 0, FbxDeformer::eSkin ) );
                FbxSkin::EType const skinningType = pSkin->GetSkinningType();
                if ( skinningType != FbxSkin::eRigid && skinningType != FbxSkin::eLinear )
                {
                    rawMesh.LogError( "Unsupported skinning type for mesh (%s), only rigid and linear skinning supported!", geometryData.m_name.c_str() );
                    return false;
                }

                // Read skeleton
                //-------------------------------------------------------------------------

                auto pSkeletonNode = FindSkeletonForSkin( pSkin );
                if ( pSkeletonNode == nullptr )
                {
                    rawMesh.LogError( "Couldn't find skeleton for mesh!", geometryData.m_name.c_str() );
                    return false;
                }

                // Check if we have already read a skeleton for this mesh and if the skeleton matches
                if ( rawMesh.m_skeleton.IsValid() )
                {
                    StringID const skeletonRootName( (char const*) pSkeletonNode->GetNameWithoutNameSpacePrefix() );
                    if ( rawMesh.GetSkeleton().GetRootBoneName() != skeletonRootName )
                    {
                        rawMesh.LogError( "Different skeletons detected for the various sub-meshes. Expected: %s, Got: %s", rawMesh.GetSkeleton().GetRootBoneName().c_str(), skeletonRootName.c_str() );
                        return false;
                    }
                }
                else // Try to read the skeleton
                {
                    Fbx::ReadSkeleton( sceneCtx, (char const*) pSkeletonNode->GetNameWithoutNameSpacePrefix(), rawMesh.m_skeleton );
                    if ( !rawMesh.m_skeleton.IsValid() )
                    {
                        rawMesh.LogError( "Failed to read mesh skeleton", geometryData.m_name.c_str() );
                        return false;
                    }
                }

                // Read skinning cluster data (i.e. bone skin mapping)
                //-------------------------------------------------------------------------

                FbxRawSkeleton& rawSkeleton = static_cast<FbxRawSkeleton&>( rawMesh.m_skeleton );

                auto const numClusters = pSkin->GetClusterCount();
                for ( auto c = 0; c < numClusters; c++ )
                {
                    FbxCluster* pCluster = pSkin->GetCluster( c );
                    FbxNode* pClusterNode = pCluster->GetLink();

                    StringID const clusterName( pClusterNode->GetNameWithoutNameSpacePrefix() );
                    if ( pClusterNode == nullptr )
                    {
                        rawMesh.LogError( "No node linked to cluster %s", (char const*) pCluster->GetNameWithoutNameSpacePrefix() );
                        return false;
                    }

                    FbxCluster::ELinkMode const mode = pCluster->GetLinkMode();
                    if ( mode == FbxCluster::eAdditive )
                    {
                        rawMesh.LogError( "Unsupported link mode for joint: %s", clusterName.c_str() );
                        return false;
                    }

                    // Find bone in skeleton that matches this cluster name
                    int32_t const boneIdx = rawMesh.m_skeleton.GetBoneIndex( clusterName );
                    if ( boneIdx == InvalidIndex )
                    {
                        rawMesh.LogError( "Unknown bone link node encountered in FBX scene (%s)", clusterName.c_str() );
                        return false;
                    }

                    //-------------------------------------------------------------------------

                    // Read bind pose
                    FbxAMatrix clusterLinkTransform;
                    pCluster->GetTransformLinkMatrix( clusterLinkTransform );
                    rawSkeleton.m_bones[boneIdx].m_globalTransform = sceneCtx.ConvertMatrixToTransform( clusterLinkTransform );

                    // Manually scale translation
                    rawSkeleton.m_bones[boneIdx].m_globalTransform.SetTranslation( rawSkeleton.m_bones[boneIdx].m_globalTransform.GetTranslation() * sceneCtx.GetScaleConversionMultiplier() );

                    // Add bone indices and weight to all influenced vertices
                    auto const* pControlPointIndices = pCluster->GetControlPointIndices();
                    auto const* pControlPointWeights = pCluster->GetControlPointWeights();
                    auto const numControlPointIndices = pCluster->GetControlPointIndicesCount();

                    for ( auto i = 0; i < numControlPointIndices; i++ )
                    {
                        EE_ASSERT( pControlPointIndices[i] < pMesh->GetControlPointsCount() );
                        if ( pControlPointWeights[i] > 0.f )
                        {
                            auto const& vertexIndices = controlPointVertexMapping[pControlPointIndices[i]];
                            for ( auto v = 0; v < vertexIndices.size(); v++ )
                            {
                                auto const vertexIdx = vertexIndices[v];
                                geometryData.m_vertices[vertexIdx].m_boneIndices.push_back( boneIdx );
                                geometryData.m_vertices[vertexIdx].m_boneWeights.push_back( (float) pControlPointWeights[i] );
                            }
                        }
                    }
                }

                // Ensure we have <= the max number of skinning influences per vertex
                //-------------------------------------------------------------------------

                bool vertexInfluencesReduced = false;
                auto const numVerts = geometryData.m_vertices.size();
                for ( auto v = 0; v < numVerts; v++ )
                {
                    auto& vert = geometryData.m_vertices[v];
                    auto numInfluences = vert.m_boneIndices.size();

                    if ( numInfluences > rawMesh.m_maxNumberOfBoneInfluences )
                    {
                        while ( vert.m_boneIndices.size() > rawMesh.m_maxNumberOfBoneInfluences )
                        {
                            // Remove lowest influence
                            int32_t smallestWeightIdx = 0;
                            for ( auto f = 1; f < vert.m_boneIndices.size(); f++ )
                            {
                                if ( vert.m_boneWeights[f] < vert.m_boneWeights[smallestWeightIdx] )
                                {
                                    smallestWeightIdx = f;
                                }
                            }

                            vert.m_boneWeights.erase( vert.m_boneWeights.begin() + smallestWeightIdx );
                            vert.m_boneIndices.erase( vert.m_boneIndices.begin() + smallestWeightIdx );
                        }

                        vertexInfluencesReduced = true;

                        // Re-normalize weights
                        float const totalWeight = vert.m_boneWeights[0] + vert.m_boneWeights[1] + vert.m_boneWeights[2] + vert.m_boneWeights[3];
                        for ( auto i = 1; i < rawMesh.m_maxNumberOfBoneInfluences; i++ )
                        {
                            vert.m_boneWeights[i] /= totalWeight;
                        }
                    }
                }

                if ( vertexInfluencesReduced )
                {
                    rawMesh.LogWarning( "More than %d skinning influences detected per bone for mesh (%s), this is not supported - influences have been reduced to %d", rawMesh.m_maxNumberOfBoneInfluences, geometryData.m_name.c_str(), rawMesh.m_maxNumberOfBoneInfluences );
                }

                //-------------------------------------------------------------------------

                rawSkeleton.CalculateLocalTransforms();
                return true;
            }
        };
    }

    //-------------------------------------------------------------------------

    TUniquePtr<RawAssets::RawMesh> Fbx::ReadStaticMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude )
    {
        return RawAssets::FbxMeshFileReader::ReadStaticMesh( sourceFilePath, meshesToInclude );
    }

    TUniquePtr<RawAssets::RawMesh> Fbx::ReadSkeletalMesh( FileSystem::Path const& sourceFilePath, TVector<String> const& meshesToInclude, int32_t maxBoneInfluences )
    {
        return RawAssets::FbxMeshFileReader::ReadSkeletalMesh( sourceFilePath, meshesToInclude, maxBoneInfluences );
    }
}