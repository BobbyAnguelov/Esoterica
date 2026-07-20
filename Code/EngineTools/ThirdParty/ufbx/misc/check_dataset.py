import os
import json
from typing import NamedTuple, Optional, List, Mapping
import glob
import re
import urllib.parse
import time
import math
import itertools
import datetime
import asyncio
import asyncio.subprocess

LATEST_SUPPORTED_DATE = "2025-06-10"

class TestModel(NamedTuple):
    fbx_path: str
    obj_path: Optional[str]
    mtl_path: Optional[str]
    mat_path: Optional[str]
    frame: Optional[int]

class TestCase(NamedTuple):
    root: str
    json_path: str
    title: str
    author: str
    license: str
    url: str
    skip: bool
    extra_files: List[str]
    models: List[TestModel]
    options: Mapping[str, List[str]]

def log(message=""):
    print(message, flush=True)

def single_file(path):
    if os.path.exists(path):
        return [path]
    else:
        return []

def strip_ext(path):
    if path.endswith(".gz"):
        path = path[:-3]
    base, _ = os.path.splitext(path)
    return base

def get_fbx_files(json_path):
    base_path = strip_ext(json_path)
    yield from single_file(f"{base_path}.fbx")
    yield from single_file(f"{base_path}.ufbx.obj")
    yield from glob.glob(f"{glob.escape(base_path)}/*.fbx")

def get_obj_files(fbx_path, flag_separator):
    base_path = strip_ext(fbx_path)
    yield from single_file(f"{base_path}.obj.gz")
    yield from single_file(f"{base_path}.obj")
    yield from glob.glob(f"{glob.escape(base_path)}{flag_separator}*.obj.gz")
    yield from glob.glob(f"{glob.escape(base_path)}{flag_separator}*.obj")

def get_mtl_files(obj_path):
    base_path = strip_ext(obj_path)
    yield from single_file(f"{base_path}.mtl")

def get_mat_files(obj_path):
    base_path = strip_ext(obj_path)
    yield from single_file(f"{base_path}.mat")

def remove_duplicate_files(paths):
    seen = set()
    for path in paths:
        base = strip_ext(path)
        if base in seen: continue
        seen.add(base)
        yield path

def gather_case_models(json_path, flag_separator):
    for fbx_path in get_fbx_files(json_path):
        for obj_path in remove_duplicate_files(get_obj_files(fbx_path, flag_separator)):
            mtl_path = next(get_mtl_files(obj_path), None)
            mat_path = next(get_mat_files(fbx_path), None)

            fbx_base = strip_ext(fbx_path)
            obj_base = strip_ext(obj_path)

            flags = obj_base[len(fbx_base) + len(flag_separator):].split("_")

            # Parse flags
            frame = None
            for flag in flags:
                m = re.match(r"frame(\d+)", flag)
                if m:
                    frame = int(m.group(1))

            yield TestModel(
                fbx_path=fbx_path,
                obj_path=obj_path,
                mtl_path=mtl_path,
                mat_path=mat_path,
                frame=frame)

        else:
            # TODO: Handle objless fbx
            pass

def as_list(v):
    return v if isinstance(v, list) else [v]

def append_unique(l, items):
    for item in items:
        if item not in l:
            l.append(item)
    return l

def append_unique_opt(options, name, items):
    options[name] = append_unique(options.get(name, []), items)

def iter_options(options):
    if not options: return [()]
    return itertools.product(*([(k,v) for v in vs] for k,vs in options.items()))

def get_field(path, desc, name, allow_unknown):
    value = desc.get(name)
    if isinstance(value, str):
        return value
    elif value is None:
        if allow_unknown:
            return None
        else:
            raise RuntimeError(f"{path}: Unknown value for '{name}', use --allow-unknown to bypass")
    else:
        raise RuntimeError(f"{path}: Bad value for '{name}': {value!r}")

def create_dataset_task(root_dir, root, filename, heavy, allow_unknown, last_supported_time):
    path = os.path.join(root, filename)

    with open(path, "rt", encoding="utf-8") as f:
        desc = json.load(f)

    flag_separator = desc.get("flagSeparator", "_")

    features = desc.get("features", [])

    extra_files = [os.path.join(root, ex) for ex in desc.get("extra-files", [])]
    options = { k: as_list(v) for k,v in desc.get("options", {}).items() }

    if heavy:
        append_unique_opt(options, "geometry-transform-handling", [
            "preserve", "helper-nodes", "modify-geometry",
        ])

        append_unique_opt(options, "inherit-mode-handling", [
            "preserve", "helper-nodes", "compensate",
        ])

        append_unique_opt(options, "space-conversion", [
            "transform-root", "adjust-transforms", "modify-geometry",
        ])

        append_unique_opt(options, "bake", [False, True])

    skip = False
    mtime = os.path.getmtime(path)
    if last_supported_time and mtime > latest_supported_time.timestamp():
        skip = True

    for feature in features:
        if feature == "geometry-transform":
            append_unique_opt(options, "geometry-transform-handling", [
                "preserve", "helper-nodes", "modify-geometry",
            ])
        elif feature == "geometry-transform-no-instances":
            append_unique_opt(options, "geometry-transform-handling", [
                "preserve", "helper-nodes", "modify-geometry", "modify-geometry-no-fallback",
            ])
        elif feature == "space-conversion":
            append_unique_opt(options, "space-conversion", [
                "transform-root", "adjust-transforms", "modify-geometry",
            ])
        elif feature == "inherit-mode":
            append_unique_opt(options, "inherit-mode-handling", [
                "preserve", "helper-nodes", "compensate",
            ])
        elif feature == "inherit-mode-no-fallback":
            append_unique_opt(options, "inherit-mode-handling", [
                "preserve", "helper-nodes", "compensate", "compensate-no-fallback",
            ])
        elif feature == "pivot":
            append_unique_opt(options, "pivot-handling", [
                "retain", "adjust-to-pivot", "adjust-to-rotation-pivot",
            ])
        elif feature == "bake":
            append_unique_opt(options, "bake", [False, True])
        elif feature == "ignore-missing-external":
            options["ignore-missing-external"] = [True]
        elif not skip:
            raise RuntimeError(f"Unknown feature: {feature}")

    models = []
    extra_files = []
    if not skip:
        models = list(gather_case_models(path, flag_separator))
        if not models:
            raise RuntimeError(f"No models found for {path}")

        extra_files = [os.path.join(root, ex) for ex in desc.get("extra-files", [])]

    yield TestCase(
        root=root_dir,
        json_path=path,
        title=get_field(path, desc, "title", allow_unknown),
        author=get_field(path, desc, "author", allow_unknown),
        license=get_field(path, desc, "license", allow_unknown),
        url=get_field(path, desc, "url", allow_unknown),
        skip=skip,
        extra_files=extra_files,
        models=models,
        options=options,
    )

def gather_dataset_tasks(root_dir, heavy, allow_unknown, last_supported_time):
    if os.path.isfile(root_dir):
        head, tail = os.path.split(root_dir)
        yield from create_dataset_task(head, head, tail, heavy, allow_unknown, last_supported_time)
        return

    for root, _, files in os.walk(root_dir):
        for filename in files:
            if not filename.endswith(".json"):
                continue
            yield from create_dataset_task(root_dir, root, filename, heavy, allow_unknown, last_supported_time)

if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser("check_dataset.py --root <root>")
    parser.add_argument("--root", help="Root directory to search for .json files")
    parser.add_argument("--host-url", default="", help="URL where the files are hosted")
    parser.add_argument("--exe", help="check_fbx.c executable")
    parser.add_argument("--verbose", action="store_true", help="Print verbose information")
    parser.add_argument("--heavy", action="store_true", help="Run heavy checks")
    parser.add_argument("--allow-unknown", action="store_true", help="Allow unknown fields")
    parser.add_argument("--include-recent", action="store_true", help="Run tests that are too recent")
    parser.add_argument("--threads", type=int, default=1, help="Number of threads to use for running")
    argv = parser.parse_args()

    host_url = argv.host_url if argv.host_url else argv.root
    host_url = host_url.rstrip("\\/")

    latest_supported_time = datetime.datetime.strptime(LATEST_SUPPORTED_DATE, "%Y-%m-%d")
    if argv.include_recent:
        latest_supported_time = None

    cases = list(gather_dataset_tasks(root_dir=argv.root, heavy=argv.heavy, allow_unknown=argv.allow_unknown, last_supported_time=latest_supported_time))

    def fmt_url(path, root=""):
        if root:
            path = os.path.relpath(path, root)
        path = path.replace("\\", "/")
        safe_path = urllib.parse.quote(path)
        return f"{host_url}/{safe_path}"

    def fmt_rel(path, root=""):
        if root:
            path = os.path.relpath(path, root)
        path = path.replace("\\", "/")
        return f"{path}"

    ok_count = 0
    test_count = 0

    case_ok_count = 0
    case_run_count = 0
    case_skip_count = 0

    num_threads = max(argv.threads, 1)

    begin_time = time.time()

    unbuffered_log = log

    async def run_case(buffered, case):
        global ok_count
        global test_count
        global case_ok_count
        global case_run_count
        global case_skip_count

        lines = []
        def buffered_log(s=""):
            lines.append(s)
        if buffered:
            log = buffered_log
        else:
            log = unbuffered_log

        title = case.title if case.title else "(unknown)"
        author = case.author if case.author else "(unknown)"
        license = case.license if case.license else "PROPRIETARY"
        log(f"== '{title}' by '{author}' ({license}) ==")
        log()

        if case.url:
            log(f"  source url: {case.url}")
        log(f"   .json url: {fmt_url(case.json_path, case.root)}")
        for extra in case.extra_files:
            log(f"   extra url: {fmt_url(extra, case.root)}")
        log()

        case_ok = True

        if case.skip:
            log("-- SKIP --")
            log()
            case_skip_count += 1
            return []

        case_run_count += 1

        for model in case.models:
            case_args = [argv.exe]
            case_args.append(model.fbx_path)

            extra = []

            if model.obj_path:
                case_args += ["--obj", model.obj_path]

            if model.mat_path:
                case_args += ["--mat", model.mat_path]

            if model.frame is not None:
                extra.append(f"frame {model.frame}")
                case_args += ["--frame", str(model.frame)]

            name = fmt_rel(model.fbx_path, case.root)

            extra_str = ""
            if extra:
                extra_str = " [" + ", ".join(extra) + "]"

            log(f"-- {name}{extra_str} --")
            log()
            log(f"    .fbx url: {fmt_url(model.fbx_path, case.root)}")
            if model.obj_path:
                log(f"    .obj url: {fmt_url(model.obj_path, case.root)}")
            if model.mtl_path:
                log(f"    .mtl url: {fmt_url(model.mtl_path, case.root)}")
            if model.mat_path:
                log(f"    .mat url: {fmt_url(model.mat_path, case.root)}")

            log()

            for opts in iter_options(case.options):
                test_count += 1

                args = case_args[:]
                for k,v in opts:
                    if v is False:
                        pass
                    elif v is True:
                        args += [f"--{k}"]
                    else:
                        args += [f"--{k}", v]

                log("$ " + " ".join(args))
                log()

                proc = await asyncio.create_subprocess_exec(
                        args[0], *args[1:],
                        stdout=asyncio.subprocess.PIPE,
                        stderr=asyncio.subprocess.PIPE)
                stdout, stderr = await proc.communicate()
                stdout = stdout.decode("utf-8", errors="replace").strip()
                stderr = stderr.decode("utf-8", errors="replace").strip()
                if stdout:
                    log(stdout)
                if stderr:
                    log(stderr)
                if proc.returncode == 0:
                    log()
                    log("-- PASS --")
                    ok_count += 1
                else:
                    log()
                    log("-- FAIL --")
                    case_ok = False
                log()

        if case_ok:
            case_ok_count += 1

        return lines

    async def run_cases_simple():
        for case in cases:
            await run_case(False, case)

    async def run_cases_threaded():
        tasks = []

        async def resolve_tasks():
            nonlocal tasks
            if not tasks: return
            done, pending = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
            tasks = list(pending)
            for task in done:
                print("\n".join(task.result()))

        for case in cases:
            if len(tasks) >= num_threads:
                await resolve_tasks()
            coro = run_case(True, case)
            task = asyncio.create_task(coro)
            tasks.append(task)
        while tasks:
            await resolve_tasks()

    if num_threads > 1:
        asyncio.run(run_cases_threaded())
    else:
        asyncio.run(run_cases_simple())

    end_time = time.time()

    log(f"{ok_count}/{test_count} files passed ({case_ok_count}/{case_run_count} test cases)")
    if case_skip_count > 0:
        if (latest_supported_time.hour, latest_supported_time.minute, latest_supported_time.second) == (0, 0, 0):
            time_str = latest_supported_time.strftime("%Y-%m-%d")
        else:
            time_str = latest_supported_time.strftime("%Y-%m-%d %H:%M:%S")
        log(f"WARNING: Skipped {case_skip_count} test cases modified after {time_str}")

    dur = int(math.ceil(end_time - begin_time))
    log(f"Total time: {int(dur/60)}min {dur%60}s")

    if ok_count < test_count:
        exit(1)
