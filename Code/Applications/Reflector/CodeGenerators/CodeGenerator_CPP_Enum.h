#pragma once
#include "Applications/Reflector/Database/ReflectionDatabase.h"
#include <sstream>

//-------------------------------------------------------------------------

using namespace EE::TypeSystem::Reflection;

//-------------------------------------------------------------------------

namespace EE::CPP::EnumGenerator
{
    void Generate( std::stringstream& codeFile, String const& exportMacro, ReflectedType const& type );
}