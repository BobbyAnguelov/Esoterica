#pragma once
#include <EASTL/functional.h>

//-------------------------------------------------------------------------

namespace EE
{
    template<typename T> using TFunction = eastl::function<T>;
}