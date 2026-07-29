# yabridge Agent Instructions

## Context management — read this first

You are running in a CI environment with a limited context window. To avoid
context overflow across long tasks:

1. **Read memory.md at the start of every session** before doing anything else.
   It contains the current state of the project and what was done last time.

2. **Write memory.md before ending your session** — even if the task is not
   complete. Record exactly where you stopped, what files you modified, and
   what needs to happen next. This is the only persistent state across runs.

3. **Keep tool output small.** When reading files, prefer `head`, `grep`, or
   targeted line ranges over `cat` on large files. Avoid printing entire files
   unless strictly necessary. Large `cat` outputs accumulate in context and
   cause sessions to fail.

4. **Commit incrementally.** After completing a logical chunk of work, run
   `git add -A && git commit -m "..."` to checkpoint. If the session ends
   early, at least the completed work is saved.

5. **Do not re-read files you already have.** If you've already read a file
   this session, don't cat it again. Use what you know.

## memory.md format

Keep memory.md structured and concise — it's read at the start of every run
and should fit in a few hundred tokens:

```
## Status
[One-line summary of current task status]

## Last modified files
- path/to/file.cpp — [what changed]

## Architecture decisions
- [key decision and why]

## Next steps
1. [specific next action]
2. [specific next action]

## Open issues
- [anything blocked or unclear]
```

## Project: yabridge

yabridge bridges Windows VST2/VST3/CLAP plugins to Linux via Wine.
The main source is in `src/`, with `src/plugin/` for the Linux plugin side
and `src/wine-host/` for the Wine host side. Build system is Meson.
