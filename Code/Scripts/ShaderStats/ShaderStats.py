import xml.etree.ElementTree as ET
import os
import re
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed
import multiprocessing

# Matches a markdown table row: leading |, cells separated by |, trailing |
_TABLE_ROW_RE = re.compile(r"^\|(.+)\|$")

# List of platforms to analyze
PLATFORMS = [
    ('gfx1100', 'RDNA 3 (RX 7900)'),
    ('gfx1150', 'RDNA 3.5 (Radeon 8040S)'),
    ('gfx1200', 'RDNA 4.0 (RX 9060)')
]

def get_rga_executable():
    """
    Get RGA executable path from environment variable.
    
    Returns:
        Path to rga.exe or None if not found
    """
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

def parse_rga_stats(shader_name, stage, platform):
    """
    Parse RGA statistics file for a shader.
    
    Args:
        shader_name: Name of the shader
        stage: Shader stage (PS or CS)
        platform: GPU platform (e.g., gfx1100)
        
    Returns:
        Dictionary containing shader statistics or None if file not found
    """
    cwd = os.getcwd()
    
    # Determine stats file path based on stage
    if stage == 'PS':
        stats_file = f"{cwd}\\Build\\RGA\\{platform}_{shader_name}.PS_pixel.txt"
    elif stage == 'CS':
        stats_file = f"{cwd}\\Build\\RGA\\{platform}_{shader_name}.CS_comp.txt"
    else:
        return None
    
    # Check if file exists
    if not os.path.exists(stats_file):
        print(f"  ⚠ Warning: Stats file not found: {stats_file}")
        return None
    
    try:
        stats = {}
        with open(stats_file, 'r') as f:
            for line in f:
                line = line.strip()
                # Skip empty lines and the "Statistics:" header
                if not line or line == 'Statistics:':
                    continue
                
                # Parse lines in format: "- key = value"
                if line.startswith('-'):
                    # Remove the leading "- " and split on "="
                    line = line[1:].strip()
                    if '=' in line:
                        key, value = line.split('=', 1)
                        key = key.strip()
                        value = value.strip()
                        
                        # Try to convert value to int, otherwise keep as string
                        try:
                            value = int(value)
                        except ValueError:
                            pass
                        
                        # Handle nested keys like "resourceUsage.numUsedVgprs"
                        if '.' in key:
                            parts = key.split('.')
                            current = stats
                            for part in parts[:-1]:
                                if part not in current:
                                    current[part] = {}
                                current = current[part]
                            current[parts[-1]] = value
                        else:
                            stats[key] = value
        
        return stats if stats else None
        
    except Exception as e:
        print(f"  ✗ Error parsing stats file: {e}")
        return None

def run_rga_analysis_wrapper(args_tuple):
    """
    Wrapper function for parallel execution of RGA analysis.
    
    Args:
        args_tuple: Tuple of (rga_exe, shader_name, stage, file_path, entry_point, dxc_options, platform, macros)
    
    Returns:
        Tuple of (shader_name, stage, platform, success)
    """
    rga_exe, shader_name, stage, file_path, entry_point, dxc_options, platform, macros = args_tuple
    success = run_rga_analysis(rga_exe, shader_name, stage, file_path, entry_point, dxc_options, platform, macros)
    return (shader_name, stage, platform, success)

def run_rga_analysis(rga_exe, shader_name, stage, file_path, entry_point, dxc_options, platform, macros):
    """
    Run RGA analysis for a shader.
    
    Args:
        rga_exe: Path to rga.exe
        shader_name: Name of the shader
        stage: Shader stage (PS or CS)
        file_path: Full path to shader file
        entry_point: Shader entry point
        dxc_options: DXC command line options
        platform: GPU platform (e.g., gfx1100)
        macros: List of macro names to define
    """
    cwd = os.getcwd()
    
    # Build RGA command arguments
    args = [
        rga_exe,
        "-s", "dx12",
        "-c", platform,
        f"--{stage.lower()}", file_path,
        f"--{stage.lower()}-entry", entry_point,
        "--dxc-opt", dxc_options,
        "-I", f"{cwd}/Code",
        "-D", "EE_RGA_ANALYZE=1"
    ]
    
    # Add macro definitions
    for macro in macros:
        args.extend(["-D", f"{macro}=1"])
    
    # Add remaining arguments
    args.extend([
        "--all-model", "6_6",
        "--autogen-dir", f"{cwd}\\Build\\RGA\\Generated\\{platform}\\{shader_name}",
        "--isa", f"{cwd}\\Build\\RGA\\ISA\\{platform}_{shader_name}.{stage}.isa",
        "-a", f"{cwd}\\Build\\RGA\\{shader_name}.{stage}.txt"
    ])
    
    print(f"Running RGA for {shader_name} ({stage}) on {platform}...")
    
    try:
        # Create shader-specific directory
        os.makedirs(f"{cwd}\\Build\\RGA\\Generated\\{platform}\\{shader_name}", exist_ok=True)
        
        # Run RGA
        result = subprocess.run(args, capture_output=True, text=True)
        
        if result.returncode == 0:
            print(f"  ✓ Success")
        else:
            print(f"  ✗ Failed with return code {result.returncode}")
            if result.stderr:
                print(f"  Error: {result.stderr}")
        
        return result.returncode == 0
        
    except Exception as e:
        print(f"  ✗ Exception: {e}")
        return False

def _format_markdown_tables(text: str) -> str:
    """Reformat all markdown tables in *text* to align columns for plaintext
    readability.  Returns the reformatted text."""
    lines = text.splitlines()
    output = []
    i = 0

    while i < len(lines):
        # Find consecutive table rows
        start = i
        while i < len(lines) and _TABLE_ROW_RE.match(lines[i]):
            i += 1

        if i - start >= 2:
            block = lines[start:i]
            output.extend(_align_table_block(block))
        else:
            if i == start:
                output.append(lines[i])
                i += 1
            else:
                output.extend(lines[start:i])

    return "\n".join(output) + "\n"


def _align_table_block(lines):
    """Reformat a single markdown table block with aligned columns."""
    rows = []
    for line in lines:
        m = _TABLE_ROW_RE.match(line)
        if not m:
            return lines
        rows.append([c.strip() for c in m.group(1).split("|")])

    if len(rows) < 2:
        return lines

    ncols = len(rows[0])
    if any(len(r) != ncols for r in rows):
        return lines

    widths = [0] * ncols
    for r in rows:
        for j, cell in enumerate(r):
            widths[j] = max(widths[j], len(cell))

    result = []
    for ri, r in enumerate(rows):
        padded = " | ".join(cell.ljust(widths[j]) for j, cell in enumerate(r))
        result.append("| " + padded + " |")

        if ri == 1 and all(c.replace("-", "").replace(":", "").strip() == "" for c in r):
            sep_cells = []
            for j, cell in enumerate(r):
                cell = cell.strip()
                left = ":" if cell.startswith(":") else ""
                right = ":" if cell.endswith(":") else ""
                sep_cells.append(left + "-" * (widths[j] - len(left) - len(right)) + right)
            result[ri] = "| " + " | ".join(sep_cells) + " |"

    return result


def xml_to_markdown_table(xml_file_path, output_file_path):
    """
    Read XML file with CompiledShader entries and convert to markdown table.
    
    Args:
        xml_file_path: Path to the input XML file
        output_file_path: Path to save the markdown table
    """
    # Get RGA executable path
    rga_exe = get_rga_executable()
    if not rga_exe:
        sys.exit(1)
    
    # Create all required directories upfront
    cwd = os.getcwd()
    for (platform, platform_name) in PLATFORMS:
        os.makedirs(f"{cwd}\\Build\\RGA\\Generated\\{platform}", exist_ok=True)
    os.makedirs(f"{cwd}\\Build\\RGA\\ISA", exist_ok=True)
    os.makedirs(os.path.dirname(output_file_path), exist_ok=True)
    
    # Parse the XML file
    tree = ET.parse(xml_file_path)
    root = tree.getroot()
    
    # Collect shader data
    shaders = []
    for shader in root.findall('.//CompiledShader'):
        name = shader.get('Name', '')
        stage = shader.get('Stage', '')
        file_path = shader.get('FilePath', '')
        entry_point = shader.get('EntryPoint', '')
        dxc_options = shader.get('CommandLineDXC', '')
        
        # Extract macro definitions
        macros = []
        for macro in shader.findall('MacroDefinition'):
            macro_name = macro.get('Name', '')
            if macro_name:
                macros.append(macro_name)
        
        # Append macros to shader permutation (PS only)
        permutation = ''
        for macro in macros:
            permutation += f"`{macro.replace('PERMUTATION_', '')}` " # Strip some stuff to fit on the screen
        
        # Only process PS and CS shaders
        if stage in ['PS', 'CS']:
            shaders.append({
                'name': name,
                'permutation': permutation,
                'stage': stage,
                'file_path': file_path,
                'entry_point': entry_point,
                'dxc_options': dxc_options,
                'macros': macros
            })
    
    # Run RGA analysis for all platforms and shaders
    print("\n>>> Running RGA Analysis\n")
    print(f"Using {multiprocessing.cpu_count()} CPU cores for parallel processing\n")
    
    # Prepare all tasks
    tasks = []
    for (platform, platform_name) in PLATFORMS:
        for shader in shaders:
            tasks.append((
                rga_exe,
                shader['name'],
                shader['stage'],
                shader['file_path'],
                shader['entry_point'],
                shader['dxc_options'],
                platform,
                shader['macros']
            ))
    
    # Run tasks in parallel
    with ProcessPoolExecutor(max_workers=multiprocessing.cpu_count()) as executor:
        futures = [executor.submit(run_rga_analysis_wrapper, task) for task in tasks]
        
        # Wait for all tasks to complete and collect results
        completed = 0
        total = len(tasks)
        for future in as_completed(futures):
            completed += 1
            shader_name, stage, platform, success = future.result()
            status = "✓" if success else "✗"
            print(f"[{completed}/{total}] {status} {shader_name} ({stage}) on {platform}")
    
    print("\n>>> All RGA Analysis Complete\n")
    
    # Generate markdown tables for each platform
    print("\n>>> Generating Markdown Tables\n")
    markdown_output = []
    
    for (platform, platform_name) in PLATFORMS:
        # Create platform-specific table header
        markdown_output.append(f"## {platform_name}")
        
        # Process each pixel shader for this platform
        markdown_output.append('')
        markdown_output.append("### Pixel shaders")
        markdown_output.append("| Name | Permutation | VGPR | SGPR  | Occupancy |")
        markdown_output.append("|------|-------------|------|-------|-----------|")
        for shader in shaders:
            if shader['stage'] != 'PS':
                continue

            stats = parse_rga_stats(shader['name'], shader['stage'], platform)
            vgprTotal = stats.get('numAvailableVgprs', 1)
            sgprTotal = stats.get('numAvailableSgprs', 1)
            
            # Only add to table if we have valid stats
            if stats:
                resource_usage = stats.get('resourceUsage', {})
                vgpr = resource_usage.get('numUsedVgprs', 0)
                sgpr = resource_usage.get('numUsedSgprs', 0)
                # ldsUsage = resource_usage.get('ldsUsageSizeInBytes', 0)
                # ldsSize = resource_usage.get('ldsSizePerLocalWorkGroup', 0)
                # scratchSize = resource_usage.get('scratchMemUsageInBytes', 0)
                
                markdown_output.append(f"| {shader['name']} | {shader['permutation']} | {vgpr}/{vgprTotal} | {sgpr}/{sgprTotal} | {(1.0 - vgpr / vgprTotal)*100.0:.2f}% |")

        # Process each compute shader for this platform
        markdown_output.append('')
        markdown_output.append("### Compute shaders\n\n")
        markdown_output.append("| Name | VGPR | SGPR  | Occupancy | LDS | Scratch |")
        markdown_output.append("|------|------|-------|-----------|-----|---------|")
        for shader in shaders:
            if shader['stage'] != 'CS':
                continue

            stats = parse_rga_stats(shader['name'], shader['stage'], platform)
            vgprTotal = stats.get('numAvailableVgprs', 1)
            sgprTotal = stats.get('numAvailableSgprs', 1)
            
            # Only add to table if we have valid stats
            if stats:
                resource_usage = stats.get('resourceUsage', {})
                vgpr = resource_usage.get('numUsedVgprs', 0)
                sgpr = resource_usage.get('numUsedSgprs', 0)
                ldsUsage = resource_usage.get('ldsUsageSizeInBytes', 0)
                ldsSize = resource_usage.get('ldsSizePerLocalWorkGroup', 0)
                scratchSize = resource_usage.get('scratchMemUsageInBytes', 0)
                
                markdown_output.append(f"| {shader['name']} | {vgpr}/{vgprTotal} | {sgpr}/{sgprTotal} | {(1.0 - vgpr / vgprTotal)*100.0:.2f}% | {ldsUsage/1024.0:.2f}/{ldsSize/1024.0:.2f}Kb | {scratchSize}Kb |")
        
        # Add spacing between tables
        markdown_output.append("")
    
    # Join all lines and format tables for plaintext readability
    markdown_table = '\n'.join(markdown_output)
    markdown_table = _format_markdown_tables(markdown_table)

    # Write to file
    with open(output_file_path, 'w') as f:
        f.write(markdown_table)
    
    print(f"\nMarkdown tables saved to {output_file_path}")
    return markdown_table

# Main execution
if __name__ == "__main__":
    input_file = "Build/ShaderStageMetadata.xml"
    output_file = "Docs/Rendering/ShaderStats.md"
    
    xml_to_markdown_table(input_file, output_file)