File: crude_v1_spec.md

# CRUDE v1 Specification

## 1. Overview

CRUDE is a filesystem-first source snapshot tool written in C and built with Pelles C.

CRUDE creates full numbered snapshots of a project folder. Each snapshot includes:

- a full copy of the tracked source tree
- a notes file named `crude_notes.txt`

The notes file contains exactly 2 sections:

- `description` - what work was completed
- `direction` - current thinking, next steps, design concerns, or warnings

CRUDE is intentionally simple. It does not attempt to replace Git.

## 2. Purpose

CRUDE exists to provide a low-ceremony workflow for local source history.

It is intended to support:

- fast local snapshots
- thought capture at the time of the snapshot
- easy comparison with external diff tools such as WinMerge
- easy manual restore by copying files from a snapshot when needed

## 3. Design philosophy

CRUDE v1 is based on these principles:

- filesystem first
- full snapshots, not change-only deltas
- no database
- no built-in diff UI
- no restore feature
- no hidden config inside the executable
- simple command-line execution
- preserve both code and thought

## 4. Scope of v1

### Included in v1

- command-line driven execution
- project name derived from source folder name
- numeric revision folders
- recursive full-copy snapshots
- multiline note entry in console
- partial snapshot cleanup on failure
- compatibility with external diff tools

### Not included in v1

- database storage
- built-in diff viewer
- restore command
- branch support
- merge support
- selective file filters
- partial or changed-files-only snapshots
- metadata preservation for copied files
- long-path support beyond normal Win32 behavior
- special handling for reparse points or junctions

## 5. Invocation

CRUDE is run from the command line with 2 required arguments:

```text
crude.exe "<source_path>" "<repo_path>"
````

### Argument definitions

* `source_path`

  * the root folder of the project to snapshot

* `repo_path`

  * the root folder where CRUDE stores snapshots

### Example

```text
crude.exe "C:\dev\XecronixEngine\FBIPursuit" "C:\CRUDE"
```

## 6. Project naming

CRUDE derives the project name from the final folder name of `source_path`.

### Example

If the source path is:

```text
C:\dev\XecronixEngine\FBIPursuit
```

then the project name is:

```text
FBIPursuit
```

CRUDE stores snapshots under: (Controlled by Command-line Arguments described later)

```text
C:\CRUDE\FBIPursuit\
```

## 7. Repository layout

The output layout for a project is:

```text
<repo_path>\
    <project_name>\
        revisions\
            000001\
            000002\
            000003\
```

### Example

```text
C:\CRUDE\
    FBIPursuit\
        revisions\
            000001\
            000002\
```

Each revision folder contains:

* `crude_notes.txt`
* a full copy of the source tree contents

## 8. Revision numbering

Revision folders are numeric and zero-padded to 6 digits.

### Format

```text
000001
000002
000003
...
999999
```

### Rules

* numbering is per project
* the next revision id is the next available integer
* v1 assumes the valid revision range is `000001` to `999999`

## 9. Notes capture

For each revision, CRUDE prompts the user for 2 multiline note blocks:

1. `description`
2. `direction`

Input is entered line by line in the console.

A single dot on its own line terminates each section.

### Input behavior

* user enters one or more lines
* entering `.` on a line by itself ends the current section
* the same rule is used for both sections

### Meaning of each tag

#### `description`

Used to describe completed work.

Examples:

* bug fixes
* refactoring performed
* feature work completed
* files reorganized
* cleanup that was finished

#### `direction`

Used to describe current thinking or future intent.

Examples:

* what should happen next
* design doubts
* warnings
* architectural direction
* experiments in progress
* mental breadcrumbs for the next session

## 10. Notes file format

Each revision contains a file named:

```text
crude_notes.txt
```

The file contains exactly 2 tagged blocks:

```text
<description>
Implemented pause menu.
Moved quit handling into WinMain.
Cleaned up restart flow.
</description>

<direction>
Need to decide whether running belongs in GameState or stays global.
Game modes are starting to feel like messages.
Next likely subsystem: actor manager.
</direction>
```

### Format rules

* both tags always exist
* tag names are fixed
* multiline content is allowed
* no additional tags are part of v1
* empty content is allowed

### Empty example

```text
<description>
</description>

<direction>
Still experimenting.
Need to test whether this approach is correct.
</direction>
```

## 11. Snapshot behavior

CRUDE creates a full snapshot of the source folder.

### Copy rules

* the entire source tree is copied as-is
* normal files and directories are copied recursively
* the destination revision folder becomes a full standalone snapshot

### Important consequence

Each revision is independently usable as a complete project copy.

This makes it easy to:

* inspect history
* diff revisions with external tools
* manually restore files by copying them back

## 12. Exclusion behavior

CRUDE v1 does not use include or exclude filters for normal project content.

The one required exclusion is:

* do not recursively copy the CRUDE destination tree into itself

### Practical meaning

If the destination repo tree is encountered during the recursive copy, it is skipped.

This prevents recursive self-copy behavior.

## 13. Filesystem traversal rules

CRUDE v1 uses normal Win32 directory walking.

### Supported

* normal files
* normal directories

### Ignored or unsupported

* reparse points
* junctions
* symbolic link style traversal cases
* long-path handling beyond normal MAX_PATH oriented behavior

## 14. Metadata preservation

CRUDE v1 does not attempt to preserve file metadata beyond creating copied files.

Not preserved as a v1 requirement:

* original timestamps
* file attributes
* ACLs
* alternate data streams
* ownership metadata

The goal is content preservation, not filesystem metadata fidelity.

## 15. Error handling

CRUDE follows an all-or-nothing snapshot rule.

### Rule

A snapshot is complete, or it does not exist.

### Behavior on failure

If snapshot creation fails:

* CRUDE reports failure
* CRUDE deletes the partial revision folder

This prevents half-finished revisions from being treated as valid history.

## 16. User workflow

Typical workflow:

1. user edits project files normally
2. user runs `crude.exe` from a command prompt or `.bat` file
3. user enters `description`
4. user enters `direction`
5. CRUDE creates the next numbered revision
6. CRUDE writes `crude_notes.txt`
7. CRUDE copies the full source tree into the new revision folder
8. user can compare revisions later using WinMerge or another external tool

## 17. External diff workflow

CRUDE v1 does not contain built-in compare or diff functionality.

Instead, it is designed to work naturally with external tools such as WinMerge.

### Example use

Compare these 2 folders:

```text
C:\CRUDE\FBIPursuit\revisions\000001
C:\CRUDE\FBIPursuit\revisions\000002
```

or compare specific files between revisions.

This approach avoids building a custom UI in v1.

## 18. Configuration model in v1

CRUDE v1 uses command-line arguments for required configuration.

### Why

* keeps config out of the executable
* works naturally with `.bat` files
* easy to change per project
* simple to understand and debug

### Example batch wrapper

```bat
@echo off
call clean.bat
C:\dev\crude\crude.exe "C:\dev\XecronixEngine\FBIPursuit" "C:\CRUDE"
```

### Future direction

Optional tools such as editors or diff tools may later be configured with environment variables, but that is not part of v1.

## 19. Build assumptions

CRUDE v1 was implemented as a Win32 console program in Pelles C.

### Build notes

* project type should be Win32 Console
* runtime entry point is `main(...)`
* console interaction is required for note entry

## 20. Non-goals

CRUDE v1 is not trying to solve the following:

* distributed version control
* collaboration
* branching
* merging
* remote hosting
* change-only storage optimization
* repository compression
* commit graph visualization
* diff UI design
* automated restore workflows

These concerns are intentionally left outside v1.

## 21. Practical advantages of v1

CRUDE v1 provides these benefits:

* very low ceremony
* full project state saved every revision
* easy manual recovery
* easy folder-to-folder diffing
* thought capture missing from most source control workflows
* simple enough to trust and inspect directly

## 22. Known limitations

As implemented in v1, these limitations are accepted:

* snapshot size grows with every full copy
* no filtering of build output or temporary files
* no database search over notes
* no timestamp metadata preservation
* no built-in revision compare command
* no automatic restore command
* no long-path support strategy
* no special handling for symlink-like traversal cases
* revision id space is finite
* all notes are entered interactively through the console

## 23. Core identity of CRUDE

CRUDE is best understood as:

* a local source snapshot tool
* a thought-tracking companion to coding
* a between-commits workflow
* a filesystem-first history system

It is not meant to replace Git.

It is meant to make local work easier, faster, and more honest.

## 24. Summary

CRUDE v1 is defined by 5 core choices:

1. full snapshots
2. filesystem-first storage
3. no database
4. no built-in diff UI
5. exactly 2 note tags:

   * `description`
   * `direction`



