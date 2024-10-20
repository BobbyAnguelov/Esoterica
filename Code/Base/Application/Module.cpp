#include "Module.h"
#include "Base/Types/Arrays.h"
#include "Base/Resource/ResourceSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    TInlineVector<EE::Resource::ResourcePtr*, 4> Module::GetModuleResources() const
    {
        return TInlineVector<EE::Resource::ResourcePtr*, 4>();
    }

    void Module::LoadModuleResources( Resource::ResourceSystem& resourceSystem )
    {
        TInlineVector<Resource::ResourcePtr*, 4> const moduleResources = GetModuleResources();
        for ( auto pResourcePtr : moduleResources )
        {
            resourceSystem.LoadResource( *pResourcePtr );
            m_currentlyLoadingResources.emplace_back( pResourcePtr );
        }

        m_hasResourceLoadingFailed = false;
    }

    Module::LoadingState Module::GetModuleResourceLoadingState()
    {
        for ( int32_t i = int32_t( m_currentlyLoadingResources.size() ) - 1; i >= 0; i-- )
        {
            LoadingStatus const resourceStatus = m_currentlyLoadingResources[i]->GetLoadingStatus();
            if ( resourceStatus == LoadingStatus::Unloaded || resourceStatus == LoadingStatus::Loading || resourceStatus == LoadingStatus::Unloading )
            {
                continue;
            }

            if ( resourceStatus == LoadingStatus::Failed )
            {
                m_hasResourceLoadingFailed = true;
            }
            else if ( resourceStatus == LoadingStatus::Loaded )
            {
                OnModuleResourceLoaded( m_currentlyLoadingResources[i] );
            }

            m_currentlyLoadingResources.erase_unsorted( m_currentlyLoadingResources.begin() + i );
        }

        // Return overall state
        //-------------------------------------------------------------------------

        if ( m_currentlyLoadingResources.empty() )
        {
            return m_hasResourceLoadingFailed ? LoadingState::Failed : LoadingState::Succeeded;
        }
        else
        {
            return LoadingState::Busy;
        }
    }

    void Module::UnloadModuleResources( Resource::ResourceSystem& resourceSystem )
    {
        EE_ASSERT( m_currentlyLoadingResources.empty() );

        TInlineVector<Resource::ResourcePtr*, 4> const moduleResources = GetModuleResources();
        for ( auto pResourcePtr : moduleResources )
        {
            OnModuleResourceUnload( pResourcePtr );
            resourceSystem.UnloadResource( *pResourcePtr );
        }

        m_hasResourceLoadingFailed = false;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Module::HotReload_UnloadResources( Resource::ResourceSystem& resourceSystem, TInlineVector<ResourceID, 20> const& resourcesToUnload )
    {
        TInlineVector<Resource::ResourcePtr*, 4> const moduleResources = GetModuleResources();
        for ( auto pResourcePtr : moduleResources )
        {
            if ( VectorContains( resourcesToUnload, pResourcePtr->GetResourceID() ) )
            {
                resourceSystem.UnloadResource( *pResourcePtr );
            }
        }
    }

    void Module::HotReload_ReloadResources( Resource::ResourceSystem& resourceSystem, TInlineVector<ResourceID, 20> const& resourcesToReload )
    {
        TInlineVector<Resource::ResourcePtr*, 4> const moduleResources = GetModuleResources();
        for ( auto pResourcePtr : moduleResources )
        {
            if ( VectorContains( resourcesToReload, pResourcePtr->GetResourceID() ) )
            {
                resourceSystem.LoadResource( *pResourcePtr );
                m_currentlyLoadingResources.emplace_back( pResourcePtr );
            }
        }
    }
    #endif
}