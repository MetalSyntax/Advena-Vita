#!/usr/bin/env python3
"""
decomp_query.py -- token-cheap search/extraction over the decompiled Advena sources.

Replaces the pattern of writing a brand-new one-off script per lookup
(tools/analysis_scripts/find_*.py, print_*.py, ...) with a single reusable CLI.
Every subcommand prints ONLY the slice of text needed to answer the question --
never a whole file -- so it can be used freely without burning context budget.

Sources searched:
  - decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c   Ghidra pseudo-C (~290k lines)
  - decompiled/libgameDSO_armeabi/ghidra/out_angr.c     angr pseudo-C (~50k lines)
  - decompiled/apk_jadx/sources/                        jadx-decompiled Java (APK)
  - ux0_data/advena/libgameDSO.so (or Advena-1.0.1/lib/armeabi/libgameDSO.so)
                                                          real ARM .so, for symbols

Subcommands:
  grep PATTERN [--root c|java|all] [--regex] [--ignore-case] [--max N] [--context N]
      Grep-like search, returns "file:line: text" only.

  func NAME [--file out_ghidra.c|out_angr.c] [--all]
      Extract one (or --all matching) C function body by name from the Ghidra/angr
      pseudo-C, e.g. `func GVUIPlayerController::ShowBtn` or `func InitialPlayerPadSet`.
      Prints just that function, brace-matched, not the surrounding file.

  javaclass NAME [--methods]
      Locate a decompiled Java class by (partial, case-insensitive) name and print
      its source path. With --methods, prints just the method signatures (not full
      bodies) so you can decide whether to read the whole file.

  symbols PATTERN [--so PATH] [--defined-only]
      Grep the dynamic symbol table of the real .so for PATTERN (regex). Uses
      pyelftools if available, otherwise falls back to arm-vita-eabi-readelf.

Examples:
  python3 tools/decomp_query.py grep "Teamstrike" --root all
  python3 tools/decomp_query.py func GVUIPlayerController::InitialPlayerPadSet
  python3 tools/decomp_query.py javaclass NeoUIControllerView --methods
  python3 tools/decomp_query.py symbols "ShowBtn|InitialPlayerPad"
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECOMP = os.path.join(ROOT, "decompiled")
GHIDRA_DIR = os.path.join(DECOMP, "libgameDSO_armeabi", "ghidra")
JAVA_SRC = os.path.join(DECOMP, "apk_jadx", "sources")

C_FILES = ["out_ghidra.c", "out_angr.c"]

SO_CANDIDATES = [
    os.path.join(ROOT, "ux0_data", "advena", "libgameDSO.so"),
    os.path.join(ROOT, "Advena-1.0.1", "lib", "armeabi", "libgameDSO.so"),
]


def _iter_c_files(which=None):
    names = [which] if which else C_FILES
    for name in names:
        path = os.path.join(GHIDRA_DIR, name)
        if os.path.isfile(path):
            yield name, path


def _iter_java_files():
    for dirpath, _dirs, files in os.walk(JAVA_SRC):
        for f in files:
            if f.endswith(".java"):
                yield os.path.join(dirpath, f)


# --------------------------------------------------------------------------
# grep
# --------------------------------------------------------------------------

def cmd_grep(args):
    flags = re.IGNORECASE if args.ignore_case else 0
    pattern = args.pattern if args.regex else re.escape(args.pattern)
    try:
        rx = re.compile(pattern, flags)
    except re.error as e:
        print(f"error: bad regex: {e}", file=sys.stderr)
        return 1

    targets = []
    if args.root in ("c", "all"):
        targets += [path for _n, path in _iter_c_files()]
    if args.root in ("java", "all"):
        targets += list(_iter_java_files())

    hits = 0
    for path in targets:
        try:
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                lines = f.readlines()
        except OSError:
            continue
        rel = os.path.relpath(path, ROOT)
        for i, line in enumerate(lines):
            if rx.search(line):
                if args.context:
                    lo = max(0, i - args.context)
                    hi = min(len(lines), i + args.context + 1)
                    print(f"--- {rel}:{i+1} ---")
                    for j in range(lo, hi):
                        marker = ">" if j == i else " "
                        print(f"{marker}{j+1}: {lines[j].rstrip()}")
                else:
                    print(f"{rel}:{i+1}: {line.rstrip()}")
                hits += 1
                if hits >= args.max:
                    print(f"... stopped at --max {args.max} matches", file=sys.stderr)
                    return 0
    if hits == 0:
        print("no matches", file=sys.stderr)
    return 0


# --------------------------------------------------------------------------
# func: extract one C function body by name from the Ghidra/angr pseudo-C
# --------------------------------------------------------------------------

SIG_RE_TEMPLATE = r"^[^\n]*\b{name}\s*\([^\n]*\)\s*$"


def _find_function_spans(lines, name):
    """Yield (start_idx, end_idx) inclusive line ranges (0-based) for each
    function definition matching `name` in the Ghidra/angr pseudo-C convention:
        [optional blank line(s)]
        [optional '// Comment' line(s)]
        <blank line>
        <signature line ending in ')'>
        <blank line>
        {
        ...body (brace-counted, ignoring string/char literal contents)...
        }
    """
    name_re = re.compile(r"\b" + re.escape(name) + r"\s*\(")
    spans = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if name_re.search(line) and line.rstrip().endswith(")"):
            # look ahead for the opening brace within the next few lines
            j = i + 1
            while j < n and lines[j].strip() == "":
                j += 1
            if j < n and lines[j].strip() == "{":
                # walk backward to include an immediately preceding "// ..." comment
                start = i
                k = i - 1
                while k >= 0 and lines[k].strip() == "":
                    k -= 1
                if k >= 0 and lines[k].strip().startswith("//"):
                    start = k

                depth = 0
                end = None
                in_str = False
                in_char = False
                for m in range(j, n):
                    text = lines[m]
                    esc = False
                    for ch in text:
                        if esc:
                            esc = False
                            continue
                        if ch == "\\" and (in_str or in_char):
                            esc = True
                            continue
                        if in_str:
                            if ch == '"':
                                in_str = False
                            continue
                        if in_char:
                            if ch == "'":
                                in_char = False
                            continue
                        if ch == '"':
                            in_str = True
                        elif ch == "'":
                            in_char = True
                        elif ch == "{":
                            depth += 1
                        elif ch == "}":
                            depth -= 1
                            if depth == 0:
                                end = m
                                break
                    if end is not None:
                        break
                if end is not None:
                    spans.append((start, end))
                    i = end + 1
                    continue
        i += 1
    return spans


def cmd_func(args):
    found_any = False
    for fname, path in _iter_c_files(args.file):
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            lines = f.readlines()
        spans = _find_function_spans(lines, args.name)
        if not spans:
            continue
        found_any = True
        to_print = spans if args.all else spans[:1]
        for start, end in to_print:
            print(f"=== {fname}:{start+1}-{end+1} ===")
            for idx in range(start, end + 1):
                print(f"{idx+1}\t{lines[idx].rstrip()}")
            print()
        if not args.all and len(spans) > 1:
            print(
                f"note: {len(spans)} definitions matched '{args.name}' in {fname}; "
                f"showing the first. Pass --all to see every match.",
                file=sys.stderr,
            )
    if not found_any:
        print(f"no function definition found matching '{args.name}'", file=sys.stderr)
        return 1
    return 0


# --------------------------------------------------------------------------
# javaclass
# --------------------------------------------------------------------------

METHOD_SIG_RE = re.compile(
    r"^\s*(?:@\w+\s*)*"
    r"(?:public|private|protected|static|final|abstract|synchronized|native)\s[^;{}=]*\([^;]*\)\s*"
    r"(?:throws\s+[\w.,\s]+)?\s*\{?\s*$"
)


def cmd_javaclass(args):
    query = args.name.lower()
    matches = []
    for path in _iter_java_files():
        base = os.path.basename(path)
        if query in base.lower() or query in path.lower():
            matches.append(path)

    if not matches:
        print(f"no Java class found matching '{args.name}'", file=sys.stderr)
        return 1

    for path in matches:
        rel = os.path.relpath(path, ROOT)
        print(f"=== {rel} ===")
        if args.methods:
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                lines = f.readlines()
            for i, line in enumerate(lines):
                stripped = line.strip()
                if not stripped or stripped.startswith("//") or stripped.startswith("*"):
                    continue
                if METHOD_SIG_RE.match(line) and "class " not in line and "interface " not in line:
                    print(f"  {i+1}: {stripped}")
    return 0


# --------------------------------------------------------------------------
# symbols
# --------------------------------------------------------------------------

def cmd_symbols(args):
    so_path = args.so
    if not so_path:
        for cand in SO_CANDIDATES:
            if os.path.isfile(cand):
                so_path = cand
                break
    if not so_path or not os.path.isfile(so_path):
        print("error: could not locate libgameDSO.so (pass --so PATH)", file=sys.stderr)
        return 1

    try:
        rx = re.compile(args.pattern, re.IGNORECASE)
    except re.error as e:
        print(f"error: bad regex: {e}", file=sys.stderr)
        return 1

    rows = _read_dynsyms_pyelftools(so_path)
    if rows is None:
        rows = _read_dynsyms_readelf(so_path)
    if rows is None:
        print(
            "error: neither pyelftools nor arm-vita-eabi-readelf/objdump are available",
            file=sys.stderr,
        )
        return 1

    hits = 0
    for addr, kind, name in rows:
        if args.defined_only and (not addr or addr == "0"):
            continue
        if rx.search(name):
            print(f"{addr}\t{kind}\t{name}")
            hits += 1
    if hits == 0:
        print("no symbols matched", file=sys.stderr)
    return 0


def _read_dynsyms_pyelftools(so_path):
    try:
        from elftools.elf.elffile import ELFFile
    except ImportError:
        return None

    rows = []
    with open(so_path, "rb") as f:
        elf = ELFFile(f)
        section = elf.get_section_by_name(".dynsym") or elf.get_section_by_name(".symtab")
        if section is None:
            return []
        for sym in section.iter_symbols():
            if not sym.name:
                continue
            addr = hex(sym["st_value"])
            kind = sym.entry["st_info"]["type"]
            rows.append((addr, kind, sym.name))
    return rows


def _read_dynsyms_readelf(so_path):
    import shutil
    import subprocess

    readelf = (
        shutil.which("arm-vita-eabi-readelf")
        or (
            os.path.expanduser("~/vitasdk/bin/arm-vita-eabi-readelf")
            if os.path.isfile(os.path.expanduser("~/vitasdk/bin/arm-vita-eabi-readelf"))
            else None
        )
        or shutil.which("readelf")
    )
    if not readelf:
        return None

    try:
        result = subprocess.run(
            [readelf, "--dyn-syms", so_path], capture_output=True, text=True, check=False
        )
    except OSError:
        return None

    rows = []
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 8 and parts[0].rstrip(":").isdigit() is False:
            continue
        if len(parts) >= 8:
            addr, kind, name = parts[1], parts[3], parts[-1]
            rows.append((addr, kind, name))
    return rows


# --------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Token-cheap search/extraction over the decompiled Advena sources.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p_grep = sub.add_parser("grep", help="grep-like search across decompiled sources")
    p_grep.add_argument("pattern")
    p_grep.add_argument("--root", choices=["c", "java", "all"], default="all")
    p_grep.add_argument("--regex", action="store_true", help="treat PATTERN as a regex")
    p_grep.add_argument("--ignore-case", "-i", action="store_true")
    p_grep.add_argument("--max", type=int, default=200)
    p_grep.add_argument("--context", type=int, default=0, help="lines of context around each hit")
    p_grep.set_defaults(func=cmd_grep)

    p_func = sub.add_parser("func", help="extract one C function body by name")
    p_func.add_argument("name", help="function or Class::method name")
    p_func.add_argument("--file", choices=C_FILES, default=None)
    p_func.add_argument("--all", action="store_true", help="print every matching definition")
    p_func.set_defaults(func=cmd_func)

    p_java = sub.add_parser("javaclass", help="locate a decompiled Java class")
    p_java.add_argument("name")
    p_java.add_argument("--methods", action="store_true", help="print method signatures only")
    p_java.set_defaults(func=cmd_javaclass)

    p_sym = sub.add_parser("symbols", help="grep the .so dynamic symbol table")
    p_sym.add_argument("pattern")
    p_sym.add_argument("--so", default=None, help="path to libgameDSO.so")
    p_sym.add_argument("--defined-only", action="store_true")
    p_sym.set_defaults(func=cmd_symbols)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
