#pragma once

#include "Applications/Reflector/Database/ReflectionDatabase.h"
#include <sstream>

//-------------------------------------------------------------------------

using namespace EE::TypeSystem::Reflection;
using namespace EE::TypeSystem;

//-------------------------------------------------------------------------

namespace EE::CPP::TypeGenerator
{
    void Generate( ReflectionDatabase const& database, std::stringstream& codeFile, String const& exportMacro, ReflectedType const& type, TVector<ReflectedType> const& parentDescs );
}