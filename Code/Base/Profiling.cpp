#include "Profiling.h"
#include "Base/FileSystem/FileSystemPath.h"

#if EE_ENABLE_SUPERLUMINAL
#include "Superluminal/PerformanceAPI.h"
#endif

#if _WIN32
#include "Base/Platform/PlatformUtils_Win32.h"
#endif

//-------------------------------------------------------------------------

namespace EE::Profiling
{
    void StartFrame()
    {
        #if EE_ENABLE_SUPERLUMINAL
        PerformanceAPI::BeginEvent( "Frame" );
        #endif

        #if EE_DEVELOPMENT_TOOLS
        OPTICK_FRAME( "EE Main" );
        #endif
    }

    void EndFrame()
    {
        #if EE_ENABLE_SUPERLUMINAL
        PerformanceAPI::EndEvent();
        #endif
    }

    void OpenProfiler()
    {
        #if _WIN32
        FileSystem::Path const profilerPath = FileSystem::Path( Platform::Win32::GetCurrentModulePath() ) + "..\\..\\..\\..\\External\\Optick\\Optick.exe";
        Platform::Win32::StartProcess( profilerPath );
        #endif
    }

    void StartCapture()
    {
        #if EE_DEVELOPMENT_TOOLS
        OPTICK_START_CAPTURE();
        #endif
    }

    void StopCapture( FileSystem::Path const& captureSavePath )
    {
        #if EE_DEVELOPMENT_TOOLS
        OPTICK_STOP_CAPTURE();
        OPTICK_SAVE_CAPTURE( captureSavePath.c_str() );
        #endif
    }
}