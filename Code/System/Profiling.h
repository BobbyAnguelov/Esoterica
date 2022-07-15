#pragma once

#include "System/Algorithm/Hash.h"

#if !EE_DEVELOPMENT_TOOLS
#define USE_OPTICK 0
#endif

#include <optick.h>

//-------------------------------------------------------------------------

namespace EE
{
    namespace FileSystem { class Path; }

    //-------------------------------------------------------------------------

    namespace Profiling
    {
        EE_SYSTEM_API void StartFrame();
        EE_SYSTEM_API void EndFrame();

        //-------------------------------------------------------------------------

        // Open the profiler application (only available on Win64)
        EE_SYSTEM_API void OpenProfiler();

        // Capture management
        EE_SYSTEM_API void StartCapture();
        EE_SYSTEM_API void StopCapture( FileSystem::Path const& captureSavePath );
    }
}

//-------------------------------------------------------------------------

#define EE_PROFILE_THREAD_START( ThreadName ) OPTICK_START_THREAD( ThreadName )

#define EE_PROFILE_THREAD_END() OPTICK_STOP_THREAD()

// Generic scopes
//-------------------------------------------------------------------------

#define EE_PROFILE_FUNCTION() OPTICK_EVENT()
#define EE_PROFILE_SCOPE( name ) OPTICK_EVENT( name )

// Tags
//-------------------------------------------------------------------------

#define EE_PROFILE_TAG( name, value ) OPTICK_TAG( name, value )

// Waits
//-------------------------------------------------------------------------

#define EE_PROFILE_WAIT( name ) OPTICK_EVENT( name, Optick::Category::Wait )

// Category scopes
//-------------------------------------------------------------------------

#define EE_PROFILE_FUNCTION_AI() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::AI )
#define EE_PROFILE_FUNCTION_ANIMATION() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Animation )
#define EE_PROFILE_FUNCTION_CAMERA() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Camera )
#define EE_PROFILE_FUNCTION_GAMEPLAY() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::GameLogic )
#define EE_PROFILE_FUNCTION_IO() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::IO )
#define EE_PROFILE_FUNCTION_NAVIGATION() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Navigation )
#define EE_PROFILE_FUNCTION_PHYSICS() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Physics )
#define EE_PROFILE_FUNCTION_RENDER() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Rendering )
#define EE_PROFILE_FUNCTION_SCENE() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Scene )
#define EE_PROFILE_FUNCTION_RESOURCE() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Streaming )
#define EE_PROFILE_FUNCTION_NETWORK() OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Network )

#define EE_PROFILE_SCOPE_AI( name ) OPTICK_EVENT( name, Optick::Category::AI )
#define EE_PROFILE_SCOPE_ANIMATION( name ) OPTICK_EVENT( name, Optick::Category::Animation )
#define EE_PROFILE_SCOPE_CAMERA( name) OPTICK_EVENT( OPTICK_FUNC, Optick::Category::Camera )
#define EE_PROFILE_SCOPE_GAMEPLAY( name) OPTICK_EVENT( OPTICK_FUNC, Optick::Category::GameLogic )
#define EE_PROFILE_SCOPE_IO( name ) OPTICK_EVENT( name, Optick::Category::IO )
#define EE_PROFILE_SCOPE_NAVIGATION( name ) OPTICK_EVENT( name, Optick::Category::Navigation )
#define EE_PROFILE_SCOPE_PHYSICS( name ) OPTICK_EVENT( name, Optick::Category::Physics )
#define EE_PROFILE_SCOPE_RENDER( name ) OPTICK_EVENT( name, Optick::Category::Rendering )
#define EE_PROFILE_SCOPE_SCENE( name ) OPTICK_EVENT( name, Optick::Category::Scene )
#define EE_PROFILE_SCOPE_RESOURCE( name ) OPTICK_EVENT( name, Optick::Category::Streaming )
#define EE_PROFILE_SCOPE_NETWORK( name ) OPTICK_EVENT( name, Optick::Category::Network )