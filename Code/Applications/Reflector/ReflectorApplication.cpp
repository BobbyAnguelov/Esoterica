#include "Base/Application/ApplicationGlobalState.h"
#include "Base/ThirdParty/cmdParser/cmdParser.h"
#include "TypeReflection/TypeReflector.h"

//-------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
    EE::ApplicationGlobalState State;

    // Set precision of cout
    //-------------------------------------------------------------------------

    std::cout.setf( std::ios::fixed, std::ios::floatfield );
    std::cout.precision( 2 );

    // Read CMD line arguments
    //-------------------------------------------------------------------------

    cli::Parser cmdParser( argc, argv );
    cmdParser.set_required<std::string>( "s", "SlnPath", "Solution Path." );
    cmdParser.set_optional<bool>( "clean", "Clean", false, "Clean Reflection Data." );
    cmdParser.set_optional<bool>( "rebuild", "rebuild", false, "Rebuild Reflection Data." );

    if ( !cmdParser.run() )
    {
        EE_LOG_ERROR( "Type System", "Reflector", "Invalid commandline arguments" );
        return 1;
    }

    // TODO: rewrite command parser to return error codes and not throw exceptions
    EE::FileSystem::Path slnPath = cmdParser.get<std::string>( "s" ).c_str();
    bool const shouldClean = cmdParser.get<bool>( "clean" );
    bool const shouldRebuild = cmdParser.get<bool>( "rebuild" );

    // Execute reflector
    //-------------------------------------------------------------------------

    std::cout << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << " * Esoterica Reflector" << std::endl;
    std::cout << "===============================================" << std::endl << std::endl;

    // Parse solution
    EE::TypeSystem::Reflection::TypeInfoReflector typeReflector( slnPath );
    if ( typeReflector.ParseSolution() )
    {
        // Clean data
        if ( shouldClean || shouldRebuild )
        {
            if ( !typeReflector.Clean() )
            {
                std::cout << std::endl << "Failed to clean: " << slnPath.c_str() << std::endl;
                EE_TRACE_MSG( "Failed to clean: %s", slnPath.c_str() );
                return 1;
            }

            if ( shouldClean )
            {
                return 0;
            }
        }

        // Incremental Build
        if ( shouldRebuild || !shouldClean )
        {
            return typeReflector.Build() ? 0 : 1;
        }
    }

    return 0;
}