# tools/

## `decomp_query.py`

Single reusable CLI for searching/extracting from the decompiled sources
(`decompiled/`) without reading whole files into context. Use this instead of
writing a new one-off `find_*.py` / `print_*.py` script (see
`analysis_scripts/` for the historical ad-hoc scripts from earlier sessions).

```
python3 tools/decomp_query.py grep "Teamstrike" --root all
python3 tools/decomp_query.py func GVUIPlayerController::InitialPlayerPadSet
python3 tools/decomp_query.py javaclass NeoUIControllerView --methods
python3 tools/decomp_query.py symbols "ShowBtn|InitialPlayerPad"
```

Run `python3 tools/decomp_query.py -h` (or `<subcommand> -h`) for full options.

## `analysis_scripts/`

Historical one-off scripts from earlier triage sessions (disassembly probes,
opcode tests, resource copying). Kept for reference/reproducibility; prefer
`decomp_query.py` for new lookups.
