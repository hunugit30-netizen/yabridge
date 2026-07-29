# Agent Instructions

## Memory

At the start of every session, read `memory.md` if it exists and use it as context for the current task.

Throughout the session, update `memory.md` continuously as you learn things — after each significant action, not just at the end. Write to it before making any commit.

`memory.md` must always contain:

- **Architecture**: key design decisions and why they were made
- **Files**: which files do what, especially non-obvious ones
- **Patterns**: conventions used in this codebase (naming, error handling, build system quirks)
- **Open tasks**: things that were started but not finished, or follow-up work identified
- **Gotchas**: things that went wrong or were surprising, so future runs don't repeat them
- **Last session**: a brief summary of what was done most recently

Append new information rather than replacing existing entries unless they are outdated. Mark outdated entries as superseded rather than deleting them.

If `memory.md` does not exist, create it before doing anything else.
## Build System

Before making any changes, read `.github/workflows/build.yml` to understand how the project is built, what dependencies are required, what build containers are used, and how artifacts are produced. Match any new code or build changes to the patterns already established there.
