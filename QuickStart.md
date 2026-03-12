
File: QuickStart.md

# CRUDE Quick Start

## Create a .bat file for your project

In your project folder, create a batch file such as `crude.bat`.

Example:

```bat
@echo off
C:\dev\crude\crude.exe "C:\dev\XecronixEngine\FBIPursuit" "C:\CRUDE"
````

What this does:

- first argument = source project folder    
- second argument = CRUDE repo root    

Example result:

```text
C:\CRUDE\FBIPursuit\revisions\000001\
```

You can run this batch file any time you want to create a new snapshot.  You'll be prompted for 2 note types, description and direction. Type as much as you'd like.  A single dot on a line by itself ends the current note section.

---

## 1. Overview

CRUDE is a filesystem-first source snapshot tool written in C and built with Pelles C.

CRUDE creates full numbered snapshots of a project folder. Each snapshot includes:

- a full copy of the tracked source tree    
- a notes file named `crude_notes.txt`    

The notes file contains exactly 2 sections:

- `description` - what work was completed    
- `direction` - current thinking, next steps, design concerns, or warnings    

CRUDE is intentionally simple. It does not attempt to replace Git.

---

## 2. Invocation

CRUDE is run from the command line with 2 required arguments:

```text
crude.exe "<source_path>" "<repo_path>"
```

### Argument definitions

- `source_path`    
    - the root folder of the project to snapshot
        
- `repo_path`    
    - the root folder where CRUDE stores snapshots        

### Example

```text
crude.exe "C:\dev\XecronixEngine\FBIPursuit" "C:\CRUDE"
```

---

## 3. Notes capture

For each revision, CRUDE prompts the user for 2 multiline note blocks:

1. `description`    
2. `direction`    

Input is entered line by line in the console.
A **single dot** on its own line terminates each note section.

### Input behavior

- user enters one or more lines    
- entering `.` on a line by itself ends the current section    
- the same rule is used for both sections    

### Meaning of each tag

#### `description`

Used to describe completed work.  (What happened)
Examples:

* bug fixes
* refactoring performed
* feature work completed
* files reorganized
* cleanup that was finished
   
#### `direction`

Used to describe current thinking or future intent. (What's on your mind)

Examples:
* what should happen next    
- design doubts    
- warnings    
- architectural direction    
- experiments in progress    
- mental breadcrumbs for the next session    

### Notes file format

Each revision contains a file named:

```text
crude_notes.txt
```

Example:

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

---

## 4. Configuration model in v1

CRUDE v1 uses command-line arguments for required configuration.

### Why

- keeps config out of the executable    
- works naturally with `.bat` files    
- easy to change per project    
- simple to understand and debug   

### Example batch wrapper

```bat
@echo off
C:\dev\crude\crude.exe "C:\dev\XecronixEngine\FBIPursuit" "C:\CRUDE"
```

---

## 5. User workflow

Typical workflow:

1. user edits project files normally    
2. user runs `crude.exe` from a command prompt or `.bat` file    
3. user enters `description`    
4. user enters `direction`    
5. CRUDE creates the next numbered revision    
6. CRUDE writes `crude_notes.txt`    
7. CRUDE copies the full source tree into the new revision folder    
8. user can compare revisions later using WinMerge or another external tool

---

## 6. Practical advantages of v1

CRUDE v1 provides these benefits:

- very low ceremony    
- full project state saved every revision    
- easy manual recovery    
- easy folder-to-folder diffing    
- thought capture missing from most source control workflows    
- simple enough to trust and inspect directly    

---

## 7. Non-goals

CRUDE v1 is not trying to solve the following:

- distributed version control    
- collaboration    
- branching    
- merging    
- remote hosting    
- change-only storage optimization    
- repository compression    
- commit graph visualization    
- diff UI design    
- automated restore workflows    

