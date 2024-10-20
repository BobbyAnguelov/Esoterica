#pragma once

#include <EASTL/shared_ptr.h>

//-------------------------------------------------------------------------

namespace EE
{
    template<typename T> using TSharedPtr = eastl::shared_ptr<T>;
}