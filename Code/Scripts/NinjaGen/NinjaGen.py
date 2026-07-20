
from ninja_syntax import Writer
from pathlib import Path

import re, copy, subprocess

REGEX_PROJ = re.compile(r'Project\("\{([^\}]+)\}"\)[\s=]+"([^\"]+)",\s"(.+proj)", "(\{[^\}]+\})"')

class Toolchain:
    def WineGCC():
        result = Toolchain()
        result.name             = 'WineGCC'
        result.compiler_c       = 'winegcc'
        result.compiler_cpp     = 'wineg++'
        result.librarian        = 'ar'
        result.linker           = 'g++'
        result.compiler_flags   = '-D__LINUX__ -g -m64 -msse4.2 -mavx -MMD -mwindows -Wno-multichar -Wno-c++20-compat'
        result.c_flags          = '-std=c11'
        result.cpp_flags        = '-std=c++17'
        result.librarian_flags  = ''
        result.linker_flags     = ''
        return result

    def GCC():
        result = Toolchain()
        result.name             = 'GCC'
        result.compiler_c       = 'gcc'
        result.compiler_cpp     = 'g++'
        result.librarian        = 'ar'
        result.linker           = 'g++'
        result.compiler_flags   = '-D__LINUX__ -g -m64 -msse4.2 -mavx -MMD'
        result.c_flags          = '-std=c11'
        result.cpp_flags        = '-std=c++17'
        result.librarian_flags  = ''
        result.linker_flags     = ''
        return result

    def Clang():
        result = Toolchain()
        result.name             = 'Clang'
        result.compiler_c       = 'clang'
        result.compiler_cpp     = 'clang++'
        result.librarian        = 'llvm-ar'
        result.linker           = 'clang++'
        result.compiler_flags   = '-D__LINUX__ -g -m64 -msse4.2 -mavx -MMD'
        result.c_flags          = '-std=c11'
        result.cpp_flags        = '-std=c++17'
        result.librarian_flags  = ''
        result.linker_flags     = ''
        return result

    def ClangCL():
        result = Toolchain()
        result.name             = 'ClangCL'
        result.compiler_c       = 'clang-cl'
        result.compiler_cpp     = 'clang-cl'
        result.librarian        = 'llvm-ar'
        result.linker           = 'clang-cl'
        result.compiler_flags   = ''
        result.c_flags          = '/std:c11'
        result.cpp_flags        = '/std:c++17'
        result.librarian_flags  = ''
        result.linker_flags     = ''
        return result

class Configuration:
    def __init__(self, name, compiler_flags, linker_flags):
        self.name = name
        self.compiler_flags = compiler_flags
        self.linker_flags = linker_flags

class Project:
    def __init__(self, project_name):
        self.name = project_name
        self.project_type = ''
        self.c_source_files = []
        self.cpp_source_files = []
        self.header_files = []
        self.project_references = []

def parse_vcxproj(filename):
    file_path = Path(filename)
    base_path = file_path.parent

    result = Project(file_path.stem)
    with open(filename) as f:
        for line in f.readlines():
            line = line.strip()

            if line.startswith('<ClCompile Include='):
                index0 = line.find('"') + 1
                index1 = line.rfind('"')

                source_file = line[index0:index1].replace('\\', '/');

                if source_file.endswith('.cpp'):
                    result.cpp_source_files.append(str(base_path.joinpath(source_file)))
                elif source_file.endswith('.c'):
                    result.c_source_files.append(str(base_path.joinpath(source_file)))
                else:
                    print(f'Unknown file {filename} {source_file}')

            if line.startswith('<ClInclude Include='):
                index0 = line.find('"') + 1
                index1 = line.rfind('"')

                source_file = line[index0:index1].replace('\\', '/');

                if source_file.endswith('.h') or source_file.endswith('.hpp') or source_file.endswith('.inl'):
                    result.header_files.append(str(base_path.joinpath(source_file)))
                else:
                    print(f'Unknown file {filename} {source_file}')

            if line.startswith('<ProjectReference Include="'):
                index0 = line.find('"') + 1
                index1 = line.rfind('"')

                reference_file = line[index0:index1].replace('\\', '/');
                reference_project = str(Path(reference_file).stem)

                result.project_references.append(reference_project)

            if line.startswith('<ConfigurationType>'):
                index0 = line.find('>') + 1
                index1 = line.rfind('<')

                project_type = line[index0:index1]
                if project_type == 'StaticLibrary' or project_type == 'DynamicLibrary':
                    result.project_type = 'Lib'
                elif project_type == 'Application':
                    result.project_type = 'Exe'
                else:
                    result.project_type = None
    return result

def parse_sln(filename):
    result = []
    with open(filename) as f:
        for line in f.readlines():
            project_path = re.findall(REGEX_PROJ, line)
            if project_path:
                project = parse_vcxproj(project_path[0][2].replace('\\', '/'))
                if project.name != 'Esoterica.Scripts.Reflect': # Special case
                    result.append(project)
    return result


def build_toolchain(build, toolchain, configurations, projects):
    build.variable(f'CompilerFlags_{toolchain.name}', toolchain.compiler_flags)
    build.variable(f'CFlags_{toolchain.name}', toolchain.c_flags)
    build.variable(f'CppFlags_{toolchain.name}', toolchain.cpp_flags)
    build.variable(f'LibrarianFlags_{toolchain.name}', toolchain.librarian_flags)
    build.variable(f'LinkerFlags_{toolchain.name}', toolchain.linker_flags)

    for configuration in configurations:
        build.variable(f'CompilerFlags_{configuration.name}', configuration.compiler_flags)
        build.variable(f'LinkerFlags_{configuration.name}', configuration.linker_flags)

        cc_rule     = f'CC_{toolchain.name}_{configuration.name}'
        cpp_rule    = f'CPP_{toolchain.name}_{configuration.name}'
        lib_rule    = f'LIB_{toolchain.name}_{configuration.name}'
        link_rule   = f'LINK_{toolchain.name}_{configuration.name}'

        build.rule(cc_rule,
            f'{toolchain.compiler_c} -MF $out.d -c $in -o $out $CompilerFlags_{toolchain.name} $CompilerFlags_{configuration.name} $CFlags_{toolchain.name}',
            None, '$out.d')
        build.rule(cpp_rule,
            f'{toolchain.compiler_c} -MF $out.d -c $in -o $out $CompilerFlags_{toolchain.name} $CompilerFlags_{configuration.name} $CppFlags_{toolchain.name}',
            None, '$out.d')
        build.rule(lib_rule, f'{toolchain.librarian} $LibrarianFlags_{toolchain.name} rcs $in $out')
        build.rule(link_rule, f'{toolchain.linker} $LinkerFlags_{toolchain.name} $LinkerFlags_{configuration.name} $in -o $out')

        for project in projects:
            object_files = []

            project_out_path = f'Build/x64_Linux_{toolchain.name}_{configuration.name}/{project.name}'
            project_obj_path = f'{project_out_path}/Obj'

            for c_source_file in project.c_source_files:
                out_file = f'{project_obj_path}/{c_source_file}.o'
                build.build(out_file, cc_rule, c_source_file)
                object_files.append(out_file)

            for cpp_source_file in project.cpp_source_files:
                out_file = f'{project_obj_path}/{cpp_source_file}.o'
                build.build(out_file, cpp_rule, cpp_source_file)
                object_files.append(out_file)

            for reference in project.project_references:
                object_files.append(f'Build/x64_Linux_{toolchain.name}_{configuration.name}/{reference}/{reference}.a')

            if project.project_type == 'Lib':
                build.build(f'{project_out_path}/{project.name}.a', lib_rule, object_files)
            elif project.project_type == 'Exe':
                build.build(f'{project_out_path}/{project.name}', link_rule, object_files)
            else:
                print(f'Unknown project type: {project.name} {project.project_type}')

all_toolchains = [
    Toolchain.WineGCC(),
    Toolchain.GCC(),
    Toolchain.Clang(),
]

esoterica_flags = '-ICode/Base/ThirdParty/EA/EABase/include/Common -ICode/Base/ThirdParty/EA/EASTL/Include -ICode/Base/ThirdParty/imgui -IExternal/Optick/include'
esoterica_flags_msvc = esoterica_flags.replace('-I', '/I')

base_configurations = [
    Configuration('Debug',      f'{esoterica_flags} -Wall -O0 -ICode -DEE_DEBUG=1',             ''),
    Configuration('Release',    f'{esoterica_flags} -Wall -O2 -ICode -DEE_RELEASE=1',           ''),
    Configuration('Shipping',   f'{esoterica_flags} -Wall -O2 -ICode -DEE_SHIPPING=1 -flto',    '-flto')
]

reflector_toolchain = all_toolchains[0]             # WineGCC
reflector_configuration = base_configurations[2]    # Shipping

compile_commands_toolchain = Toolchain.ClangCL()
compile_commands_warnings = (
    '-Wall',
    ' -Wno-c++98-compat -Wno-c++98-compat-pedantic'
)
compile_commands_configurations = [
    Configuration('Debug', f'{esoterica_flags_msvc} {compile_commands_warnings} /ICode /DEE_DEBUG=1', ''),
]

all_configurations = []
all_configurations.extend(base_configurations)

for configuration in base_configurations:
    asan_configuration = copy.deepcopy(configuration)
    asan_configuration.name += '_ASan'
    asan_configuration.compiler_flags += ' -fsanitize-address -fno-omit-frame-pointer'
    asan_configuration.linker_flags += ' -fsanitize-address'

    msan_configuration = copy.deepcopy(configuration)
    msan_configuration.name += '_MSan'
    msan_configuration.compiler_flags += ' -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer'
    msan_configuration.linker_flags += ' -fsanitize=memory'

    tsan_configuration = copy.deepcopy(configuration)
    tsan_configuration.name += '_TSan'
    tsan_configuration.compiler_flags += ' -fsanitize=thread'
    tsan_configuration.linker_flags += ' -fsanitize=thread'

    all_configurations.append(asan_configuration)
    all_configurations.append(msan_configuration)
    all_configurations.append(tsan_configuration)

all_projects = parse_sln('Esoterica.sln')

Path('Build/x64_Linux').mkdir(parents = True, exist_ok = True)

with open('Build/x64_Linux/Esoterica.x64.Linux.ninja', 'w') as build_output:
    build = Writer(build_output, 110)

    for toolchain in all_toolchains:
        build_toolchain(build, toolchain, all_configurations, all_projects)

    # Special case - reflector to generate reflection metadata
    reflector_project = None
    autogenerated_files = []
    reflector_dependencies = set()
    module_pattern = str(Path('_Module/_AutoGenerated/_module.cpp'))
    for project in all_projects:
        if project.name == 'Esoterica.Applications.Reflector':
            reflector_project = project
        for source_file in project.cpp_source_files:
            reflector_dependencies.update(project.header_files)
            if source_file.endswith(module_pattern):
                autogenerated_files.append(source_file)

    reflector_dependencies.update(reflector_project.c_source_files)
    reflector_dependencies.update(reflector_project.cpp_source_files)

    build.rule('REFLECT',
        f'Build/x64_Linux_{reflector_toolchain.name}_{reflector_configuration.name}/{reflector_project.name}/{reflector_project.name} -s Esoterica.sln')
    build.build(autogenerated_files, 'REFLECT', list(reflector_dependencies))
    build.close()

with open('Build/x64_Linux/Esoterica.x64.CompileCommands.ninja', 'w') as build_output:
    build = Writer(build_output, 110)
    build_toolchain(build, compile_commands_toolchain, compile_commands_configurations, all_projects)
    build.close()

subprocess.run(
    'ninja -f Build/x64_Linux/Esoterica.x64.CompileCommands.ninja -t compdb > compile_commands.json',
    shell = True, check = True);
