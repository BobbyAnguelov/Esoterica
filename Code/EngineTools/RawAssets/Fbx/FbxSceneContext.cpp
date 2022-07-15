#include "FbxSceneContext.h"
#include "System/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Fbx
{
    FbxSceneContext::FbxSceneContext( FileSystem::Path const& filePath )
    {
        if ( !FileSystem::Exists( filePath ) )
        {
            m_error.sprintf( "Specified FBX file doesn't exist: %s", filePath.c_str() );
            return;
        }

        // Import the FBX file
        //-------------------------------------------------------------------------

        m_pManager = FbxManager::Create();

        auto pImporter = FbxImporter::Create( m_pManager, "EE Importer" );
        if ( !pImporter->Initialize( filePath, -1, m_pManager->GetIOSettings() ) )
        {
            m_error.sprintf( "Failed to load specified FBX file ( %s ) : %s", filePath.c_str(), pImporter->GetStatus().GetErrorString() );
            return;
        }

        m_pScene = FbxScene::Create( m_pManager, "ImportScene" );
        if ( !pImporter->Import( m_pScene ) )
        {
            m_error.sprintf( "Failed to load specified FBX file ( %s ) : %s", filePath.c_str(), pImporter->GetStatus().GetErrorString() );
            pImporter->Destroy();
            return;
        }
        pImporter->Destroy();

        // Co-ordinate system scene conversion
        //-------------------------------------------------------------------------

        auto& globalSettings = m_pScene->GetGlobalSettings();

        FbxAxisSystem const originalAxisSystem = globalSettings.GetAxisSystem();

        int32_t sign = 0;
        switch ( originalAxisSystem.GetUpVector( sign ) )
        {
            case FbxAxisSystem::eXAxis : 
            {
                m_originalUpAxis = sign >= 0 ? Axis::X : Axis::NegX;
            }
            break;

            case FbxAxisSystem::eYAxis:
            {
                m_originalUpAxis = sign >= 0 ? Axis::Y : Axis::NegY;
            }
            break;

            case FbxAxisSystem::eZAxis:
            {
                m_originalUpAxis = sign >= 0 ? Axis::Z : Axis::NegZ;
            }
            break;
        }

        if ( originalAxisSystem != FbxAxisSystem::MayaZUp || originalAxisSystem != FbxAxisSystem::Max )
        {
            FbxAxisSystem::MayaZUp.DeepConvertScene( m_pScene );
        }

        // Unit conversion
        //-------------------------------------------------------------------------
        // DO NOT USE THE FBX UNIT CONVERSION FUNCTIONS AS THEY JUST INTRODUCE SCALES INTO ALL TRANSFORMS!

        FbxSystemUnit const originalSystemUnits = globalSettings.GetSystemUnit();
        m_scaleConversionMultiplier = (float) originalSystemUnits.GetConversionFactorTo( FbxSystemUnit::m );
    }

    FbxSceneContext::~FbxSceneContext()
    {
        if ( m_pScene != nullptr ) m_pScene->Destroy();
        if ( m_pManager != nullptr ) m_pManager->Destroy();
    }

    //-------------------------------------------------------------------------

    static void FindAllNodes( FbxNode* pCurrentNode, FbxNodeAttribute::EType nodeType, TVector<FbxNode*>& results )
    {
        EE_ASSERT( pCurrentNode != nullptr );

        // Read node attribute
        //-------------------------------------------------------------------------

        FbxNodeAttribute::EType attribType = FbxNodeAttribute::EType::eUnknown;

        FbxNodeAttribute* pNodeAttribute = pCurrentNode->GetNodeAttribute();
        if ( pNodeAttribute != nullptr )
        {
            attribType = pNodeAttribute->GetAttributeType();
        }

        // Return node or continue search
        //-------------------------------------------------------------------------

        if ( attribType == nodeType )
        {
            results.emplace_back( pCurrentNode );
        }
        else // Search children
        {
            for ( auto i = 0; i < pCurrentNode->GetChildCount(); i++ )
            {
                FindAllNodes( pCurrentNode->GetChild( i ), nodeType, results );
            }
        }
    }

    void FbxSceneContext::FindAllNodesOfType( FbxNodeAttribute::EType nodeType, TVector<FbxNode*>& results ) const
    {
        EE_ASSERT( IsValid() );
        auto pRootNode = m_pScene->GetRootNode();
        FindAllNodes( pRootNode, nodeType, results );
    }

    static FbxNode* FindNodeByName( FbxNode* pCurrentNode, FbxNodeAttribute::EType nodeType, char const* pNodeNameToFind )
    {
        EE_ASSERT( pCurrentNode != nullptr && pNodeNameToFind != nullptr );

        // Read node attribute
        //-------------------------------------------------------------------------

        FbxNodeAttribute::EType attribType = FbxNodeAttribute::EType::eUnknown;

        FbxNodeAttribute* pNodeAttribute = pCurrentNode->GetNodeAttribute();
        if ( pNodeAttribute != nullptr )
        {
            attribType = pNodeAttribute->GetAttributeType();
        }

        // Return node or continue search
        //-------------------------------------------------------------------------

        if ( attribType == nodeType && pCurrentNode->GetNameWithoutNameSpacePrefix() == pNodeNameToFind )
        {
            return pCurrentNode;
        }
        else // Search children
        {
            for ( auto i = 0; i < pCurrentNode->GetChildCount(); i++ )
            {
                FbxNode* pResultNode = FindNodeByName( pCurrentNode->GetChild( i ), nodeType, pNodeNameToFind );
                if ( pResultNode != nullptr )
                {
                    return pResultNode;
                }
            }
        }

        return nullptr;
    }

    FbxNode* FbxSceneContext::FindNodeOfTypeByName( FbxNodeAttribute::EType nodeType, char const* pNodeNameToFind ) const
    {
        EE_ASSERT( IsValid() );
        auto pRootNode = m_pScene->GetRootNode();
        return FindNodeByName( pRootNode, nodeType, pNodeNameToFind );
    }

    //-------------------------------------------------------------------------

    void FbxSceneContext::FindAllAnimStacks( TVector<FbxAnimStack*>& results ) const
    {
        EE_ASSERT( IsValid() );

        auto const numAnimStacks = m_pScene->GetSrcObjectCount<FbxAnimStack>();
        for ( auto i = 0; i < numAnimStacks; i++ )
        {
            results.push_back( m_pScene->GetSrcObject<FbxAnimStack>( i ) );
        }
    }

    FbxAnimStack* FbxSceneContext::FindAnimStack( char const* pTakeName ) const
    {
        EE_ASSERT( IsValid() && pTakeName != nullptr );

        auto const numAnimStacks = m_pScene->GetSrcObjectCount<FbxAnimStack>();
        if ( numAnimStacks == 0 )
        {
            return nullptr;
        }

        for ( auto i = 0; i < numAnimStacks; i++ )
        {
            auto pAnimStack = m_pScene->GetSrcObject<FbxAnimStack>( i );
            if ( pAnimStack->GetNameWithoutNameSpacePrefix() == pTakeName )
            {
                return pAnimStack;
            }
        }

        return nullptr;
    }
}