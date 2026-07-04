#!/usr/bin/env python3
"""
Generate compile_commands.json for clangd (CMake + Ninja).

Visual Studio generators do not emit compile_commands.json; this script configures
a Ninja build tree for that purpose only.

- On Windows: MSVC via vswhere + vcvars64.bat (same idea as ClangdCompileDb.bat).
- On Linux: default CC/CXX from the environment (gcc/clang).

Run on each host OS once; outputs go to clangd-compiledb/windows or
clangd-compiledb/linux. Optionally refreshes .clangd to point at that directory.

# 自动识别当前系统
python generate_clangd_compiledb.py

# 指定源码 / 输出根目录
python generate_clangd_compiledb.py -S /path/to/Server -B /path/to/out-root

# 不写 .clangd（只生成 JSON）
python generate_clangd_compiledb.py --no-update-clangd

# 额外 CMake 参数（示例）
python generate_clangd_compiledb.py --cmake-arg=-DENABLE_LUAJIT=ON

"""

from __future__ import annotations

import argparse
import locale
import os
import shutil
import subprocess
import sys
from pathlib import Path


def _repo_root() -> Path:
    return Path(__file__).resolve().parent


def _detect_platform() -> str:
    if sys.platform == "win32":
        return "windows"
    if sys.platform.startswith("linux"):
        return "linux"
    raise SystemExit(f"Unsupported platform: {sys.platform!r} (expected Windows or Linux)")


def _which(name: str) -> str:
    path = shutil.which(name)
    if not path:
        raise FileNotFoundError(f"Not found on PATH: {name}")
    return path


def _vs_installation_path() -> Path:
    pf = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = Path(pf) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.is_file():
        raise FileNotFoundError(f"vswhere.exe not found: {vswhere}")
    proc = subprocess.run(
        [
            str(vswhere),
            "-latest",
            "-products",
            "*",
            "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property",
            "installationPath",
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    out = proc.stdout.strip()
    if not out:
        raise RuntimeError(
            "No Visual Studio installation with MSVC x64 tools found (vswhere returned empty)."
        )
    return Path(out)


def _parse_cmd_set_output(raw: bytes) -> dict[str, str]:
    enc = locale.getpreferredencoding(False) or "utf-8"
    text = raw.decode(enc, errors="replace")
    env: dict[str, str] = {}
    for line in text.splitlines():
        line = line.rstrip("\r\n")
        if "=" not in line:
            continue
        key, _, value = line.partition("=")
        env[key] = value
    return env


def _msvc_environment(vcvars: Path) -> dict[str, str]:
    """Run vcvars64.bat in cmd and capture merged environment (PATH, INCLUDE, LIB, etc.)."""
    # Use shell=True: a single argv to cmd /c gets mangled on Windows when the path contains spaces.
    comspec = os.environ.get("ComSpec") or os.environ.get("COMSPEC") or "cmd.exe"
    proc = subprocess.run(
        f'call "{vcvars}" && set',
        shell=True,
        executable=comspec,
        check=True,
        capture_output=True,
    )
    parsed = _parse_cmd_set_output(proc.stdout)
    merged = os.environ.copy()
    merged.update(parsed)
    return merged


def _run_cmake_windows(source: Path, build: Path, extra_cmake: list[str]) -> None:
    _which("cmake")
    _which("ninja")
    vs = _vs_installation_path()
    vcvars = vs / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    if not vcvars.is_file():
        raise FileNotFoundError(f"vcvars64.bat not found: {vcvars}")

    build.mkdir(parents=True, exist_ok=True)
    env = _msvc_environment(vcvars)
    cmake_args = [
        "cmake",
        "-S",
        str(source),
        "-B",
        str(build),
        "-G",
        "Ninja",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DCMAKE_C_COMPILER=cl",
        "-DCMAKE_CXX_COMPILER=cl",
        "--preset vcpkg-static",
    ]
    cmake_args.extend(extra_cmake)
    subprocess.run(cmake_args, check=True, env=env)


def _run_cmake_linux(source: Path, build: Path, extra_cmake: list[str]) -> None:
    _which("cmake")
    _which("ninja")
    build.mkdir(parents=True, exist_ok=True)
    cmd = [
        "cmake",
        "-S",
        str(source),
        "-B",
        str(build),
        "-G",
        "Ninja",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "--preset linux-vcpkg-static",
    ]
    cmd.extend(extra_cmake)
    subprocess.run(cmd, check=True, env=os.environ.copy())


def _write_clangd_config(repo: Path, compilation_db_dir: Path) -> None:
    rel = compilation_db_dir.relative_to(repo)
    # clangd accepts forward slashes on Windows
    posix = rel.as_posix()
    text = f"""CompileFlags:
  CompilationDatabase: {posix}
"""
    clangd_path = repo / ".clangd"
    clangd_path.write_text(text, encoding="utf-8")
    print(f"Wrote {clangd_path} -> CompilationDatabase: {posix}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate Ninja + compile_commands.json for clangd (Windows MSVC or Linux)."
    )
    parser.add_argument(
        "-S",
        "--source-dir",
        type=Path,
        default=None,
        help="CMake source directory (default: directory containing this script)",
    )
    parser.add_argument(
        "-B",
        "--build-root",
        type=Path,
        default=None,
        help="Parent directory for per-OS build dirs (default: <repo>/clangd-compiledb)",
    )
    parser.add_argument(
        "--platform",
        choices=("auto", "windows", "linux"),
        default="auto",
        help="Target platform layout under build root (default: detect from OS)",
    )
    parser.add_argument(
        "--no-update-clangd",
        action="store_true",
        help="Do not write .clangd",
    )
    parser.add_argument(
        "--cmake-arg",
        action="append",
        default=[],
        metavar="ARG",
        help="Extra CMake argument (repeatable), e.g. --cmake-arg=-DENABLE_LUAJIT=ON",
    )
    args = parser.parse_args()

    repo = _repo_root()
    source = (args.source_dir or repo).resolve()
    build_root = (args.build_root or (repo / "clangd-compiledb")).resolve()

    plat = args.platform if args.platform != "auto" else _detect_platform()
    if plat == "linux" and sys.platform == "win32":
        raise SystemExit(
            "Refusing to generate Linux compile DB on Windows (CMake would still target Windows). "
            "Run this script on Linux, or use WSL with the same repo path if you set that up yourself."
        )
    if plat == "windows" and not sys.platform == "win32":
        raise SystemExit(
            "Refusing to generate Windows MSVC compile DB on non-Windows. "
            "Run this script on Windows with VS + Ninja installed."
        )

    build_dir = build_root # / plat
    # 如果build_dir不存在，则创建
    if not build_dir.exists():
        build_dir.mkdir(parents=True, exist_ok=True)
    else:
        # 如果build_dir存在，则删除
        if build_dir.is_dir():
            shutil.rmtree(build_dir)
        build_dir.mkdir(parents=True, exist_ok=True)

    print(f"CMake source: {source}")
    print(f"CMake build:  {build_dir}")
    print(f"Platform:     {plat}")

    if plat == "windows":
        _run_cmake_windows(source, build_dir, args.cmake_arg)
    else:
        _run_cmake_linux(source, build_dir, args.cmake_arg)

    json_path = build_dir / "compile_commands.json"
    if not json_path.is_file():
        print(f"Warning: expected file missing: {json_path}", file=sys.stderr)
        return 1

    print(f"OK: {json_path}")
    if not args.no_update_clangd:
        _write_clangd_config(repo, build_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
