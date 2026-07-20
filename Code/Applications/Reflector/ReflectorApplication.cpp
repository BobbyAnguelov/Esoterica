#include "Reflector.h"
#include "Base/Application/ApplicationGlobalState.h"
#include "Base/Utils/CommandLineParser.h"

#include <windows.h>
#include <locale>
#include <consoleapi2.h>

//-------------------------------------------------------------------------

using namespace EE;

//-------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
    ApplicationGlobalState State;

    // Set precision of cout
    //-------------------------------------------------------------------------

    std::cout.setf( std::ios::fixed, std::ios::floatfield );
    std::cout.precision( 2 );

    // Read CMD line arguments
    //-------------------------------------------------------------------------

    CommandLineParser cl;
    cl.AddRequiredStringArg( "s", "Solution Path." );
    cl.AddOptionalBoolArg( "clean", "Clean Shader and Type Reflection Data.", false );
    cl.AddOptionalBoolArg( "rebuild", "Rebuild Shaders and Type Reflection Data.", false );
    cl.AddOptionalBoolArg( "hotreload", "Faster option for hot-reloading shaders, only valid with -shaders", false );
    cl.AddOptionalBoolArg( "shaders", "Build only Shader Data.", false );
    cl.AddOptionalBoolArg( "typeinfo", "Build only Type Reflection Data.", false );

    if ( !cl.Parse( argc, argv ) )
    {
        EE_LOG_ERROR( LogCategory::TypeSystem, "Reflector", "Invalid commandline arguments" );
        return 1;
    }

    // TODO: rewrite command parser to return error codes and not throw exceptions
    FileSystem::Path slnPath = cl.GetStringArg( "s" );

    if ( slnPath.Length() == 0 )
    {
        EE_LOG_ERROR( LogCategory::TypeSystem, "Reflector", "Invalid commandline arguments: solution path cannot be empty!" );
        return 1;
    }

    if ( !slnPath.IsValid() || slnPath.IsDirectoryPath() )
    {
        EE_LOG_ERROR( LogCategory::TypeSystem, "Reflector", "Invalid solution file name: %s", slnPath.c_str() );
        return 1;
    }

    if ( !slnPath.MatchesExtension( "slnx" ) )
    {
        EE_LOG_ERROR( LogCategory::TypeSystem, "Reflector", "Invalid solution file type: %s", slnPath.c_str() );
        return 1;
    }

    if ( !slnPath.Exists() )
    {
        EE_LOG_ERROR( LogCategory::TypeSystem, "Reflector", "Solution doesn't exist: %s", slnPath.c_str() );
        return 1;
    }

    // TODO: evaluate better alternatives to this
    //-------------------------------------------------------------------------

    bool const buildOnlyShaders = cl.GetBoolArg( "shaders" );
    bool const buildOnlyTypeinfo = cl.GetBoolArg( "typeinfo" );
    if ( buildOnlyShaders && buildOnlyTypeinfo )
    {
        EE_LOG_ERROR( LogCategory::TypeSystem, "Reflector", "Invalid commandline arguments: 'shaders' and 'typeinfo' arguments are mutally exclusive" );
        return 1;
    }

    Reflection::Reflector::Output output = Reflection::Reflector::Output::ShadersAndTypeInfo;
    if ( buildOnlyShaders )
    {
        output = Reflection::Reflector::Output::Shaders;
    }
    else if ( buildOnlyTypeinfo )
    {
        output = Reflection::Reflector::Output::TypeInfo;
    }

    //-------------------------------------------------------------------------

    Reflection::Reflector::Operation operation = Reflection::Reflector::Operation::IncrementalBuild;
    {
        bool const isCleanRequested = cl.GetBoolArg( "clean" );
        bool const isRebuildRequested = cl.GetBoolArg( "rebuild" );
        bool const isHotReloadRequested = cl.GetBoolArg( "hotreload" );

        if ( ( uint8_t( isCleanRequested ) + uint8_t( isRebuildRequested ) + uint8_t( isHotReloadRequested ) ) > 1 )
        {
            EE_LOG_ERROR( LogCategory::TypeSystem, "Reflector", "Invalid commandline arguments: 'rebuild', 'clean' and 'hotreload' arguments are mutally exclusive" );
            return 1;
        }

        if ( isCleanRequested )
        {
            operation = Reflection::Reflector::Operation::Clean;
        }
        else if ( isRebuildRequested )
        {
            operation = Reflection::Reflector::Operation::Rebuild;
        }
        else if ( isHotReloadRequested )
        {
            if ( output != Reflection::Reflector::Output::Shaders )
            {
                EE_LOG_ERROR( LogCategory::TypeSystem, "Reflector", "Invalid commandline arguments: 'hotreload' requires '-Shaders' argument as well" );
                return 1;
            }

            operation = Reflection::Reflector::Operation::HotReload;
        }
    }
    //-------------------------------------------------------------------------

    Reflection::Reflector reflector( slnPath );

    if ( !reflector.Run( output, operation ) )
    {
        return 1;
    }

    return 0;
}