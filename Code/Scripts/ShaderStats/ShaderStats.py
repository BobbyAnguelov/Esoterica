"""
Analyze Esoterica's shaders with the Radeon GPU Analyzer (RGA).

Reads Build/ShaderStageMetadata.xml (written by EsotericaReflector.exe), compiles
every shader variant for every target AMD architecture with RGA, and writes the
resource usage tables to Docs/Rendering/ShaderStats.md.

Requires the RGA_PATH environment variable to point at rga.exe.
RGA 2.15 or newer is required.
"""

import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from concurrent.futures import ProcessPoolExecutor, as_completed
import multiprocessing

_TABLE_ROW_RE = re.compile(r"^\|(.+)\|$")

PLATFORMS = [
    ('gfx1100', 'RDNA 3 (RX 7900)'),
    ('gfx1150', 'RDNA 3.5 (Radeon 890M)'),
    ('gfx1200', 'RDNA 4.0 (RX 9060)')
]

MINIMUM_RGA_VERSION = (2, 15)
SHADER_MODEL = '6_6'
RGA_OUTPUT_DIRECTORY = os.path.join('Build', 'RGA')

STAGES = {
    'MS': {'entry_point': 'MS_main', 'output_suffix': 'mesh'},
    'VS': {'entry_point': 'VS_main', 'output_suffix': 'vert'},
    'PS': {'entry_point': 'PS_main', 'output_suffix': 'pixel'},
    'CS': {'entry_point': 'CS_main', 'output_suffix': 'comp'}
}

PERMUTATION_PREFIX = 'PERMUTATION_'

SHADER_TYPE_MATERIAL = 'Material'
SHADER_TYPE_SURFACE = 'Surface'
SHADER_TYPE_COMPUTE = 'Compute'

GRAPHICS_SHADER_TYPES = [SHADER_TYPE_MATERIAL, SHADER_TYPE_SURFACE]

VS_MS_STAGES = ['MS', 'VS']

_CSV_STAT_KEYS = {
    'AVAILABLE_VGPRs': 'vgprAvailable',
    'USED_VGPRs': 'vgprUsed',
    'AVAILABLE_SGPRs': 'sgprAvailable',
    'USED_SGPRs': 'sgprUsed',
    'AVAILABLE_LDS_BYTES': 'ldsAvailableBytes',
    'USED_LDS_BYTES': 'ldsUsedBytes',
    'SCRATCH_MEM': 'scratchBytes'
}

_NOT_AVAILABLE = ('', 'N/A', '-')


def get_rga_executable():
    rga_path = os.environ.get('RGA_PATH')

    if not rga_path:
        print("ERROR: RGA_PATH environment variable is not defined!")
        print("Please set RGA_PATH to the full path of rga.exe")
        return None

    if not os.path.exists(rga_path):
        print(f"ERROR: RGA executable not found at: {rga_path}")
        print("Please check that RGA_PATH points to a valid rga.exe file")
        return None

    return rga_path


def get_rga_version(rga_exe):
    """Read the major and minor version of an RGA executable, or None."""
    try:
        result = subprocess.run([rga_exe, '--version'], capture_output=True, text=True)
    except OSError as e:
        print(f"ERROR: Could not run {rga_exe}: {e}")
        return None

    match = re.search(r'Version:\s*(\d+)\.(\d+)', result.stdout or '')
    if not match:
        return None

    return (int(match.group(1)), int(match.group(2)))


def stats_file_path(platform, label, stage):
    """
    Path RGA writes the resource usage statistics of one compiled stage to.
    """
    suffix = STAGES[stage]['output_suffix']

    return os.path.join(RGA_OUTPUT_DIRECTORY, f'{platform}_{label}_{suffix}.csv')


def _to_number(value):
    """Convert a statistic to int or float, or None when it does not apply."""
    value = value.strip()

    if value in _NOT_AVAILABLE:
        return None

    try:
        return int(value)
    except ValueError:
        pass

    try:
        return float(value)
    except ValueError:
        return value


def parse_rga_stats(stats_file):
    """
    Parse the resource usage RGA wrote for one shader stage
    """
    if not os.path.exists(stats_file):
        return None

    try:
        with open(stats_file, 'r', encoding='utf-8-sig') as f:
            lines = [line.strip() for line in f if line.strip()]
    except OSError as e:
        print(f"  x Error reading stats file: {e}")
        return None

    if len(lines) < 2:
        return None

    names = [name.strip() for name in lines[0].split(',')]
    values = [value.strip() for value in lines[1].split(',')]

    stats = {}
    for name, value in zip(names, values):
        key = _CSV_STAT_KEYS.get(name)
        if key:
            stats[key] = _to_number(value)

    return stats or None


def _infer_shader_type(shader):
    if 'CS' in shader['stages']:
        return SHADER_TYPE_COMPUTE

    if shader['permutations']:
        return SHADER_TYPE_MATERIAL

    return SHADER_TYPE_SURFACE


def _permutation_names(permutation):
    """Strip the reflector's prefix from each permutation macro."""
    prefix_length = len(PERMUTATION_PREFIX)

    return [
        macro[prefix_length:] if macro.startswith(PERMUTATION_PREFIX) else macro
        for macro in permutation
    ]


def _variant_label(name, permutation):
    """Build the unique label a shader variant is compiled and reported under."""
    return '_'.join([name] + _permutation_names(permutation))


def _permutation_text(permutation):
    """Build the permutation column of a markdown table row."""
    return ' '.join(f'`{name}`' for name in _permutation_names(permutation))


def collect_shader_variants(xml_file_path):
    """
    Group the reflector's per-stage metadata into the shader variants to compile.
    """
    root = ET.parse(xml_file_path).getroot()

    shaders = []
    index = {}

    for entry in root.findall('.//CompiledShader'):
        name = entry.get('Name', '')
        stage = entry.get('Stage', '')
        file_path = entry.get('FilePath', '')
        entry_point = entry.get('EntryPoint', '')

        if stage not in STAGES:
            continue

        is_null_stage = not entry_point

        # Skip entry points that are not interesting, for example outline ID passes
        if not is_null_stage and entry_point != STAGES[stage]['entry_point']:
            continue

        shader = index.get((name, file_path))
        if shader is None:
            shader = {
                'name': name,
                'file_path': file_path,
                'dxc_options': entry.get('CommandLineDXC', ''),
                'declared_type': entry.get('Type', ''),
                'stages': {},
                'null_stages': {},
                'permutations': []
            }
            index[(name, file_path)] = shader
            shaders.append(shader)

        macros = tuple(
            macro.get('Name', '')
            for macro in entry.findall('MacroDefinition')
            if macro.get('Name', '')
        )

        if is_null_stage:
            shader['null_stages'].setdefault(macros, set()).add(stage)
        else:
            shader['stages'][stage] = entry_point

        # Only material shaders have permutations
        if (is_null_stage or macros) and macros not in shader['permutations']:
            shader['permutations'].append(macros)

    variants = []
    for shader in shaders:
        shader_type = shader['declared_type'] or _infer_shader_type(shader)
        shader_variants = []

        for permutation in (shader['permutations'] or [()]):
            stages = dict(shader['stages'])
            null_stages = shader['null_stages'].get(permutation, set())

            for stage in null_stages:
                stages.pop(stage, None)

            shader_variants.append({
                'name': shader['name'],
                'type': shader_type,
                'label': _variant_label(shader['name'], permutation),
                'permutation': _permutation_text(permutation),
                'file_path': shader['file_path'],
                'dxc_options': shader['dxc_options'],
                'stages': stages,
                'null_stages': null_stages,
                'macros': list(permutation)
            })

        compiled = [variant for variant in shader_variants if not variant['null_stages']]
        shared_label = compiled[0]['label'] if compiled else None

        for variant in shader_variants:
            if variant['null_stages'] and shared_label is not None:
                variant['stats_labels'] = {stage: shared_label for stage in variant['stages']}
            else:
                variant['stats_labels'] = {stage: variant['label'] for stage in variant['stages']}

        variants.extend(shader_variants)

    return variants


def find_unknown_stages(xml_file_path):
    """
    Find stages in the metadata this tool does not know how to compile.
    """
    root = ET.parse(xml_file_path).getroot()

    return sorted({
        entry.get('Stage', '')
        for entry in root.findall('.//CompiledShader')
        if entry.get('Stage', '') and entry.get('Stage', '') not in STAGES
    })


def build_rga_command(rga_exe, platform, variant, generated_dir, stats_base, isa_base):
    """Build the RGA command line that compiles and analyzes one shader variant."""
    cwd = os.getcwd()

    args = [rga_exe, '-s', 'dx12', '-c', platform, '--offline']

    for stage in STAGES:
        entry_point = variant['stages'].get(stage)
        if entry_point is None:
            continue

        args.extend([f"--{stage.lower()}", variant['file_path']])
        args.extend([f"--{stage.lower()}-entry", entry_point])

    if variant['dxc_options']:
        args.extend(['--dxc-opt', variant['dxc_options']])

    args.extend(['-I', f"{cwd}/Code"])

    for macro in variant['macros']:
        args.extend(['-D', f'{macro}=1'])

    args.extend([
        '--all-model', SHADER_MODEL,
        '--autogen-dir', generated_dir,
        '--isa', isa_base,
        '-a', stats_base
    ])

    return args


def run_rga_analysis(task):
    """
    Run RGA once for one shader variant on one platform.
    """
    for path in task['stats_files'].values():
        if os.path.exists(path):
            os.remove(path)

    os.makedirs(task['generated_dir'], exist_ok=True)

    output = ''
    try:
        result = subprocess.run(task['command'], capture_output=True, text=True)
        output = (result.stdout or '') + (result.stderr or '')
    except OSError as e:
        output = f'{e}'

    missing = [
        os.path.basename(path) for path in task['stats_files'].values() if not os.path.exists(path)
    ]

    return {
        'platform': task['platform'],
        'label': task['label'],
        'missing': missing,
        'output': output if missing else ''
    }


def get_shader_stage_metadata(rga_exe):
    try:
        result = subprocess.run([rga_exe, '-h', '-s', 'dx12'], capture_output=True, text=True)
    except OSError:
        return set(STAGES)

    help_text = (result.stdout or '') + (result.stderr or '')

    return {
        stage for stage in STAGES
        if re.search(rf'^\s*--{stage.lower()}\s', help_text, re.MULTILINE)
    }


def _align_table_block(lines):
    """Reformat a single markdown table block with aligned columns."""
    rows = []
    for line in lines:
        match = _TABLE_ROW_RE.match(line)
        if not match:
            return lines
        rows.append([cell.strip() for cell in match.group(1).split('|')])

    if len(rows) < 2:
        return lines

    ncols = len(rows[0])
    if any(len(row) != ncols for row in rows):
        return lines

    widths = [0] * ncols
    for row in rows:
        for column_index, cell in enumerate(row):
            widths[column_index] = max(widths[column_index], len(cell))

    result = []
    for row_index, row in enumerate(rows):
        padded = ' | '.join(cell.ljust(widths[i]) for i, cell in enumerate(row))
        result.append('| ' + padded + ' |')

        # Second row is the separator, align its dashes with the column widths
        if row_index == 1 and all(c.replace('-', '').replace(':', '').strip() == '' for c in row):
            separators = []
            for i, cell in enumerate(row):
                cell = cell.strip()
                left = ':' if cell.startswith(':') else ''
                right = ':' if cell.endswith(':') else ''
                separators.append(left + '-' * (widths[i] - len(left) - len(right)) + right)
            result[row_index] = '| ' + ' | '.join(separators) + ' |'

    return result


def _format_markdown_tables(text):
    """Align the columns of every markdown table in *text* for readability."""
    lines = text.splitlines()
    output = []
    i = 0

    while i < len(lines):
        start = i
        while i < len(lines) and _TABLE_ROW_RE.match(lines[i]):
            i += 1

        if i - start >= 2:
            output.extend(_align_table_block(lines[start:i]))
        elif i == start:
            output.append(lines[i])
            i += 1
        else:
            output.extend(lines[start:i])

    return '\n'.join(output) + '\n'


def _dash(value):
    """Format a statistic that may not apply to this stage."""
    return '-' if value is None else str(value)


def _register_cell(stats, used_key, available_key):
    """Format a "used/available" register column."""
    if not stats:
        return '-'

    used = stats.get(used_key)
    available = stats.get(available_key)

    if used is None and available is None:
        return '-'

    return f'{_dash(used)}/{_dash(available)}'


def _memory_cell(stats, used_key, available_key, unit):
    """Format a "used/available" memory column in the given unit."""
    if not stats:
        return '-'

    used = stats.get(used_key)
    if used is None:
        return '-'

    available = stats.get(available_key)
    available_text = '-' if available is None else f'{available / 1024.0:.2f}'

    return f'{used / 1024.0:.2f}/{available_text}{unit}'


def _occupancy_cell(stats):
    """Share of the VGPR file left over after one wave of this shader."""
    if not stats:
        return '-'

    available = stats.get('vgprAvailable')
    used = stats.get('vgprUsed')

    if not available or used is None:
        return '-'

    return f'{(1.0 - used / available) * 100.0:.2f}%'


def _variant_stage_text(variant):
    """Name the stages of one pipeline, for example "MS + PS"."""
    return ' + '.join(stage for stage in STAGES if stage in variant['stages'])


def _variant_stats(variant, platform, stage):
    """Read the statistics of one stage of a variant."""
    return parse_rga_stats(
        stats_file_path(platform, variant['stats_labels'].get(stage, variant['label']), stage)
    )


def _build_graphics_section(platform, variants, shader_type):
    """
    Build the markdown table of one graphics shader type for one platform.
    """
    rows = [
        variant for variant in variants
        if variant['type'] == shader_type
        and any(stage in variant['stages'] for stage in VS_MS_STAGES)
    ]

    include_permutation = any(variant['permutation'] for variant in rows)

    columns = ['Name']
    if include_permutation:
        columns.append('Permutation')
    columns.extend([
        'Stages', 'VS/MS VGPR', 'VS/MS SGPR', 'LDS', 'PS VGPR', 'PS SGPR'
    ])

    lines = [f'### {shader_type} shaders', '']
    lines.append('| ' + ' | '.join(columns) + ' |')
    lines.append('|' + ' | '.join('---' for _ in columns) + '|')

    for variant in rows:
        vs_ms_stage = next(stage for stage in VS_MS_STAGES if stage in variant['stages'])

        vs_ms = _variant_stats(variant, platform, vs_ms_stage)
        ps = _variant_stats(variant, platform, 'PS') if 'PS' in variant['stages'] else None

        cells = [variant['name']]
        if include_permutation:
            cells.append(variant['permutation'])

        cells.append(_variant_stage_text(variant))

        if not vs_ms and not ps:
            cells.append('(no RGA stats)')
            cells.extend('-' for _ in range(len(columns) - len(cells)))
            lines.append('| ' + ' | '.join(cells) + ' |')
            continue

        cells.extend([
            _register_cell(vs_ms, 'vgprUsed', 'vgprAvailable'),
            _register_cell(vs_ms, 'sgprUsed', 'sgprAvailable'),
            _memory_cell(vs_ms, 'ldsUsedBytes', 'ldsAvailableBytes', 'Kb')
        ])

        if 'PS' in variant['null_stages']:
            cells.extend(['-', '-'])
        else:
            cells.extend([
                _register_cell(ps, 'vgprUsed', 'vgprAvailable'),
                _register_cell(ps, 'sgprUsed', 'sgprAvailable')
            ])

        lines.append('| ' + ' | '.join(cells) + ' |')

    return lines


def _build_compute_section(platform, variants):
    """Build the markdown table of the compute shaders of one platform."""
    lines = ['### Compute shaders', '']
    lines.append('| Name | VGPR | SGPR | Occupancy | LDS | Scratch |')
    lines.append('|------|------|------|-----------|-----|---------|')

    for variant in variants:
        if variant['type'] != SHADER_TYPE_COMPUTE or 'CS' not in variant['stages']:
            continue

        stats = _variant_stats(variant, platform, 'CS')

        if not stats:
            lines.append(f"| {variant['name']} | - | - | (no RGA stats) | - | - |")
            continue

        scratch_bytes = stats.get('scratchBytes')

        lines.append(
            f"| {variant['name']}"
            f" | {_register_cell(stats, 'vgprUsed', 'vgprAvailable')}"
            f" | {_register_cell(stats, 'sgprUsed', 'sgprAvailable')}"
            f" | {_occupancy_cell(stats)}"
            f" | {_memory_cell(stats, 'ldsUsedBytes', 'ldsAvailableBytes', 'Kb')}"
            f" | {'-' if scratch_bytes is None else f'{scratch_bytes}B'} |"
        )

    return lines


def build_markdown(variants):
    """Build the markdown document: one section per shader type and platform."""
    lines = []

    for platform, platform_name in PLATFORMS:
        lines.append(f"## {platform_name}")
        lines.append("")

        for shader_type in GRAPHICS_SHADER_TYPES:
            lines.extend(_build_graphics_section(platform, variants, shader_type))
            lines.append("")

        lines.extend(_build_compute_section(platform, variants))
        lines.append("")

    return _format_markdown_tables('\n'.join(lines))


def build_tasks(rga_exe, variants):
    """Prepare one RGA invocation per shader variant and platform."""
    tasks = []

    for platform, platform_name in PLATFORMS:
        for variant in variants:
            if variant['null_stages']:
                continue

            generated_dir = os.path.abspath(
                os.path.join(RGA_OUTPUT_DIRECTORY, 'Generated', platform, variant['label'])
            )

            # RGA adds the platform prefix and the stage suffix to these
            stats_base = os.path.abspath(os.path.join(RGA_OUTPUT_DIRECTORY, variant['label']))
            isa_base = os.path.abspath(
                os.path.join(RGA_OUTPUT_DIRECTORY, 'ISA', variant['label'])
            )

            tasks.append({
                'platform': platform,
                'label': variant['label'],
                'generated_dir': generated_dir,
                'stats_files': {
                    stage: os.path.abspath(stats_file_path(platform, variant['label'], stage))
                    for stage in variant['stages']
                },
                'command': build_rga_command(
                    rga_exe, platform, variant, generated_dir, stats_base, isa_base
                )
            })

    return tasks


def run_tasks(tasks):
    """Run every prepared RGA invocation, returning the ones that produced no statistics."""
    failures = []

    with ProcessPoolExecutor(max_workers=multiprocessing.cpu_count()) as executor:
        futures = [executor.submit(run_rga_analysis, task) for task in tasks]

        completed = 0
        for future in as_completed(futures):
            completed += 1
            result = future.result()
            success = not result['missing']
            status = 'v' if success else 'x'
            print(f"[{completed}/{len(tasks)}] {status} {result['label']} on {result['platform']}")

            if not success:
                failures.append(result)

    print("\n>>> All RGA Analysis Complete\n")

    if failures:
        print(f">>> {len(failures)} of {len(tasks)} compilations produced no statistics\n")

        for failure in failures[:10]:
            print(f"--- {failure['label']} on {failure['platform']} ---")
            for line in failure['output'].splitlines():
                if 'error' in line.lower():
                    print(f"    {line.strip()}")
            print(f"    missing: {', '.join(failure['missing'])}")
            print("")

        if len(failures) > 10:
            print(f"... and {len(failures) - 10} more\n")

    return failures


def main(xml_file_path, output_file_path):
    rga_exe = get_rga_executable()
    if not rga_exe:
        sys.exit(1)

    version = get_rga_version(rga_exe)
    if version is None:
        print(f"ERROR: Could not read the RGA version from: {rga_exe}")
        sys.exit(1)

    print(f"Using RGA {version[0]}.{version[1]} at {rga_exe}")

    if version < MINIMUM_RGA_VERSION:
        print(f"ERROR: RGA {MINIMUM_RGA_VERSION[0]}.{MINIMUM_RGA_VERSION[1]} or newer is required.")
        print("Mesh shader pipelines are only analyzed by newer releases.")
        sys.exit(1)

    supported_stages = get_shader_stage_metadata(rga_exe)

    cwd = os.getcwd()
    os.makedirs(os.path.join(cwd, RGA_OUTPUT_DIRECTORY, 'ISA'), exist_ok=True)
    for platform, platform_name in PLATFORMS:
        os.makedirs(os.path.join(cwd, RGA_OUTPUT_DIRECTORY, 'Generated', platform), exist_ok=True)
    os.makedirs(os.path.dirname(output_file_path), exist_ok=True)

    unknown_stages = find_unknown_stages(xml_file_path)
    if unknown_stages:
        print(f"WARNING: Ignoring unknown shader stages in the metadata: {', '.join(unknown_stages)}")
        print("Add them to STAGES to report them.")

    variants = collect_shader_variants(xml_file_path)

    stages_used = set()
    for variant in variants:
        stages_used.update(variant['stages'].keys())

    unsupported = sorted(stages_used - supported_stages)
    if unsupported:
        print(f"ERROR: This RGA cannot compile: {', '.join(unsupported)}")
        sys.exit(1)

    tasks = build_tasks(rga_exe, variants)

    print("\n>>> Running RGA Analysis\n")
    print(f"Using {multiprocessing.cpu_count()} CPU cores for parallel processing")
    print(f"{len(variants)} shader variants across {len(PLATFORMS)} platforms = {len(tasks)} compilations\n")

    failures = run_tasks(tasks)

    print("\n>>> Generating Markdown Tables\n")

    markdown_table = build_markdown(variants)

    with open(output_file_path, 'w') as f:
        f.write(markdown_table)

    print(f"\nMarkdown tables saved to {output_file_path}")

    if failures:
        sys.exit(1)


if __name__ == "__main__":
    main("Build/ShaderStageMetadata.xml", "Docs/Rendering/ShaderStats.md")
