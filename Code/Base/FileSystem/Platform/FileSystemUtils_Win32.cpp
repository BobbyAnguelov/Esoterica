#ifdef _WIN32
#include "../FileSystemUtils.h"
#include "Base/Platform/PlatformUtils_Win32.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    Path GetCurrentProcessPath()
    {
        return Path( EE::Platform::Win32::GetCurrentModulePath() ).GetParentDirectory();
    }
}
#endif