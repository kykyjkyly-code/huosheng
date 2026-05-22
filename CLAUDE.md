# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

RoboCup Small Size League (SSL) soccer robot skills project. Builds Windows DLLs that plug into the **SOM v3.4.2 (SoccerPlanner3)** simulation platform. Each DLL exports a single `player_plan` function called by the simulator every frame to control a robot.

## Build Commands

VS2013 (v120) toolchain is required. VS2013 is installed at `D:\Downloads\VC`.

**Debug build:**
```powershell
cd D:\huosheng\testskill
cmd.exe /c 'call "D:\Downloads\VC\vcvarsall.bat" x86 && "C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" testskill.sln /t:Rebuild /p:Configuration=Debug /p:Platform=Win32 /m'
```

**Release build:**
```powershell
cd D:\huosheng\testskill
cmd.exe /c 'call "D:\Downloads\VC\vcvarsall.bat" x86 && "C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" testskill.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /m'
```

DLL output: `D:\huosheng\testskill\Debug\` or `Release\`. Copy to `SOM v3.4.2\Team\user_skills\` for use.

The project links against a prebuilt static lib: `testskill\worldmodel_lib\worldmodel_lib.lib`.

## Architecture

```
testskill/testskill/           ← main project
  *.cpp                        ← skill files (mutually exclusive via #if guards)
  src/utils/                   ← shared headers (PlayerTask, WorldModel, maths, vector, etc.)
  src/getballsource.h/.cpp     ← shared GetBall helper class
  guanfang/                    ← original class-based official SOM skills (reference)
testskill/SOM v3.4.2/          ← simulation platform (SoccerPlanner3.exe, Lua scripts, DLLs)
```

### Skill File Convention

Only **one** `player_plan` export can be active per DLL. Each `.cpp` skill file starts with:

```cpp
#if 1    // ← change to 1 to activate this skill
```
or
```cpp
#if 0    // ← change to 0 to deactivate
```

All other skill files must be `#if 0`. Always use `/t:Rebuild` after switching to avoid stale `.obj` files.

Every skill defines the same entry point:
```cpp
extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);
```

### Key Types

- **`PlayerTask`** (`src/utils/PlayerTask.h`): Output struct — target position, orientation, velocity, kick/suction control
- **`WorldModel`** (`src/utils/worldmodel.h`): Read-only world state — ball, robots, game state
- **`point2f`** (`src/utils/vector.h`): 2D float vector, used everywhere for positions
- **`Maths`** / **`FieldPoint`** (`src/utils/maths.h`): Angle normalization, geometry utilities, field coordinates

### Conventions

- **Simulation vs real mode**: Skills branch on `model->get_simulation()` to pick `SIM_*` or `REAL_*` parameter sets
- **Static state**: Skills use `static` local variables for frame-to-frame state persistence
- **Zero-based robot IDs**: Arrays of size 6, IDs 0–5
- **Null guard**: Every `player_plan` starts with `if (model == NULL) return task;`
- **Angle normalization**: All orientation math goes through `Maths::normalizeAngle()`
- **DLL name**: Configured via `<TargetName>` in the `.vcxproj` (currently `goalie_timing_shoot`)

### Common Build Errors

- **Duplicate `player_plan`**: Multiple `.cpp` files have `#if 1`. Only keep one.
- **C2014**: `#` not at column 0 — hidden BOM/whitespace at file start. Delete leading characters.
- **LNK4099 / C4819**: Warnings only, do not block the build.
