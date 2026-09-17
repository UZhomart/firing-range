# Firing Range

> **This is an educational project.** It was written as a learning exercise for
> the Firing Range assignment and is not a commercial product. The code is
> written and commented so that it shows how the systems of a first person
> shooter fit together, rather than for brevity.

**Engine:** Unreal Engine 5.5
**Language:** C++ only, no Blueprints
**Platforms:** Windows, Linux, macOS

*[Русская версия](README.md)*

### Authors

| Author | Area of responsibility |
|---|---|
| **zutemiss** | Project skeleton and engine configuration, core types and persistence, character and input, the whole weapon system, ballistics and projectile physics, ammunition crates, session rules and accuracy accounting |
| **dshadykh** | Targets and hit zones, moving target AI and patrol routes, the HUD, every Slate menu, the procedural range builder, documentation |

The split is visible in the history: `git shortlog -sne` and `git log --author=...`.

A first person firing range: three firearms, physics driven bullets, stationary
and AI driven moving targets, ammunition crates, a HUD that reports accuracy, a
main menu on its own map, a pause menu and a timed challenge mode.

---

## Contents

1. [Ready-made build](#ready-made-build) — download and play without installing the engine
2. [What to download](#what-to-download) — to work with the source code
3. [Installation](#installation)
4. [Other platforms](#other-platforms) — macOS and Linux
5. [Controls](#controls)
6. [Repository structure](#repository-structure)
7. [What makes this project unusual](#what-makes-this-project-unusual)
8. [How it works](#how-it-works)
9. [Assignment checklist](#assignment-checklist)

---

## Ready-made build

The packaged Windows game lives in the repository on **GitHub**. It is there
rather than on the school Gitea because the archive weighs about 260 MB, and Git LFS,
which files of that size need, is disabled on Gitea.

| | |
|---|---|
| **File page** | https://github.com/UZhomart/firing-range/blob/main/Release/FiringRange-Win64.zip |
| **Direct link** | https://github.com/UZhomart/firing-range/raw/main/Release/FiringRange-Win64.zip |
| **Size** | 260 MB zipped, 380 MB extracted |
| **Configuration** | Shipping, Windows 64-bit |

> The build is Windows only. For macOS and Linux, see
> [Other platforms](#other-platforms).

### How to run it

1. Download the archive from the direct link, or open the file page and press
   the download icon on the right (**Download raw file**).
2. Extract it: right click the archive → **Extract All…** → **Extract**.
3. Open the `FiringRange-Win64` folder that appears and run **`FiringRange.exe`**.
4. If Windows shows *"Windows protected your PC"*, press **More info** →
   **Run anyway**. The build is not code signed, so SmartScreen does not
   recognise it.
5. If an error mentions `MSVCP140.dll` or `VCRUNTIME140.dll`, run
   `Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe` from the same folder. It
   installs the missing Microsoft libraries. Then run `FiringRange.exe` again.

### What the computer needs

- Windows 10 or 11, 64-bit
- A graphics card with DirectX 11 or 12 support and an up to date driver

Unreal Engine and Visual Studio are **not** needed to run the build. They are
only needed to work with the source code, which the sections below cover.

---

## What to download

| What | Link | Size | Why |
|---|---|---|---|
| **Visual Studio 2022 Community** | https://visualstudio.microsoft.com/vs/community/ | ~10–12 GB | The C++ compiler. Install it **first** |
| **Epic Games account** | https://www.epicgames.com/id/register | — | Needed to download the engine, free |
| **Unreal Engine 5.5** | https://www.unrealengine.com/en-US/download | ~35–40 GB | The engine itself |
| Git LFS *(optional)* | https://git-lfs.com | ~10 MB | So that cloning the GitHub repository also downloads the build archive |

**Disk space:** about 63 GB free (engine + Visual Studio + build artefacts).

---

## Installation

### 1. Visual Studio 2022

Download it from the link above → run `VisualStudioSetup.exe` → in the workload
selection tick two workloads:

- ☑ **Game development with C++**
- ☑ **Desktop development with C++**

Nothing else is needed. Press **Install**.

> The order matters: the Unreal installer looks for the MSVC compiler when it
> runs. If Visual Studio is not there yet, the engine installs without C++
> support.

### 2. Unreal Engine 5.5

1. Download and install the **Epic Games Launcher** from the link above.
2. Sign in with your Epic account.
3. Pick the **Unreal Engine** tab on the left and **Library** at the top.
4. Under *Engine Versions*, press the yellow **`+`**.
5. On the new slot, press the **down arrow** next to the version number and
   choose **5.5.x**. A newer version is offered by default, and it will not do.
6. Press **Install**, then the **Options** link in the dialog, and untick
   *Starter Content*, *Templates and Feature Packs*, *Engine Source*, and every
   platform except Windows.
7. Install.

> **If the Engine Versions list stays empty** and **Install Engine** does
> nothing on a freshly created Epic account, wait a few hours. Access to the
> engine is granted to new accounts with a delay.

### 3. The project

```bash
git clone https://01.tomorrow-school.ai/git/zutemiss/firing-range.git
cd firing-range
```

Double click **`FiringRange.uproject`**. The editor asks whether to build the
missing modules — answer **Yes**. The first compilation takes 5–15 minutes.

> **If Windows asks "How do you want to open this file?"**, choose nothing and
> close the dialog. It means `.uproject` files are not associated with Unreal on
> that machine. Open the project with the command from
> [Starting the game](#5-starting-the-game), option B.

### 4. The two maps

The maps are already in the repository: `Content/Maps/MainMenu.umap` and
`Content/Maps/FiringRange.umap`. Both are **empty** — everything in them is
built in code at startup — and each already points at its game mode.

There is nothing to do. The steps below are only for the case where the maps
have gone missing.

**Restore with the script:** launch the editor so that it runs the script by
itself:

```powershell
& "C:/Program Files/Epic Games/UE_5.5/Engine/Binaries/Win64/UnrealEditor.exe" `
  "<project>/FiringRange.uproject" `
  -ExecutePythonScript="<project>/Scripts/GenerateMaps.py"
```

Or, in an editor that is already open: `Window → Output Log`, switch the console
at the bottom to **Python**, and run:

```python
exec(open(r"<project>/Scripts/GenerateMaps.py").read())
```

**Restore by hand:**

1. `File → New Level → Empty Level` → `File → Save Current Level As…` →
   `Content/Maps/MainMenu`
2. Repeat, saving as `Content/Maps/FiringRange`

### 5. Starting the game

There are two ways. The commands below are for PowerShell and an engine
installed in the default location. If the project lives elsewhere, replace
`C:\Users\python\Desktop\firing-range` with your own path.

#### Option A — straight into the game, without the editor

The game opens in its own window, like any other game. The main menu comes
first.

```powershell
& "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\python\Desktop\firing-range\FiringRange.uproject" -game -windowed -ResX=1280 -ResY=720
```

The `-game` flag runs the project as a game, and `-windowed -ResX -ResY` set a
1280×720 window. To avoid typing the command every time, make a shortcut: right
click the desktop → **New → Shortcut** → paste the same line without the leading
`& ` → **Next** → name it `Firing Range` → **Finish**.

#### Option B — through the editor

1. Open the project in the editor:

   ```powershell
   & "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\python\Desktop\firing-range\FiringRange.uproject"
   ```

2. Wait for the editor window. On the first launch a *Compiling Shaders* counter
   runs in the bottom right corner — it can take 10–40 minutes.
3. Press the green **▶ Play** triangle in the top toolbar (or `Alt+P`).
4. Stop the game with `Esc`, then the **■ Stop** button in the toolbar.

#### What happens next

1. The **FIRING RANGE** main menu opens — the separate `MainMenu` map.
2. **START GAME** loads the `FiringRange` map, and the game mode builds the whole
   range before the player is given a pawn.
3. To quit: `Esc` → **MAIN MENU** → **QUIT**.

Nothing has to be assigned in the editor: no Blueprint has to be created, no
class has to be picked in a dropdown, and no actor has to be dragged into a
level.

#### On a weak machine

With integrated graphics or 8 GB of memory, lower the quality in the editor:
the **⚙ Settings** button in the top right → **Engine Scalability Settings** →
**Low**. For option A, append `-ExecCmds="scalability 0"` to the command.

---

## Other platforms

The ready-made build above is **Windows only**. It will not run on macOS or
Linux.

### macOS

A separate Mac build can only be made on a Mac: Unreal cannot build a game for
macOS from Windows, because that needs Xcode. On top of that, an unsigned
application on macOS has to be opened with right click → **Open**.

What the Mac needs:

- **Xcode** from the App Store
- **Unreal Engine 5.5** for macOS — through the Epic Games Launcher, the same way
  as in [Installation](#2-unreal-engine-55)

**Run the project without packaging** — the same as option A on Windows:

```bash
cd firing-range
"/Users/Shared/Epic Games/UE_5.5/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/FiringRange.uproject" -game -windowed -ResX=1280 -ResY=720
```

On the first launch the engine builds the project's C++ module for macOS, which
takes a few minutes.

**Build the `.app` application:**

```bash
"/Users/Shared/Epic Games/UE_5.5/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$PWD/FiringRange.uproject" -noP4 -platform=Mac -clientconfig=Shipping \
  -build -cook -stage -pak -compressed -archive -archivedirectory="$PWD/Packaged"
```

The application appears in `Packaged/Mac/FiringRange.app`. If macOS says the
application is damaged or cannot be verified, remove its quarantine attribute:

```bash
xattr -cr Packaged/Mac/FiringRange.app
```

> The code was written to be cross platform, but the project has not been built
> on macOS yet. clang is stricter than the Visual Studio compiler, so the first
> build may need small fixes.

### Linux

A Linux build can be made directly on Windows. It needs Epic's cross-compile
toolchain — clang for Linux, in the version listed in the UE 5.5 requirements.
Once it is installed, `-platform=Win64` in the packaging command becomes
`-platform=Linux`.

**A Linux build will not run on a Mac.** macOS is not Linux: they use different
executable formats (Mach-O and ELF), different system libraries and different
graphics APIs (Metal and Vulkan). A Mac needs its own build — see above.

---

## Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move |
| Mouse | Look |
| `Shift` | Sprint |
| `Ctrl` or `C` | Crouch |
| `Space` | Jump |
| **Left mouse** | Fire |
| **Right mouse** | Aim down sights |
| `R` | Reload |
| `1` `2` `3` | Pistol, shotgun, sniper rifle |
| Mouse wheel | Cycle weapons |
| `T` | Start a timed challenge |
| `Esc` | Pause |

A gamepad is mapped to the same actions.

Walk over an ammunition crate to restock. A crate is only consumed when the
rounds actually fit, so walking over one with a full reserve leaves it standing.

---

## Repository structure

```
firing-range/
├── FiringRange.uproject          project manifest
├── README.md                     Russian documentation
├── README.en.md                  this file
├── firing-range__ts.md           assignment brief
├── .gitignore  .gitattributes    git rules
├── Config/                       engine configuration
├── Content/Maps/                 two empty maps
├── Release/                      packaged build archive (GitHub only)
├── Scripts/                      helper scripts
├── resources/                    images from the brief
└── Source/                       source code
```

### Root

| File | Description |
|---|---|
| `FiringRange.uproject` | Engine version (5.5), module list, enabled plugins |
| `README.md` / `README.en.md` | Documentation in Russian and English |
| `firing-range__ts.md` | The original brief, kept so the result can be checked against it |
| `.gitignore` | Excludes the folders Unreal generates (`Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`) and the personal study notes in `learn/` |
| `.gitattributes` | Line endings are always LF — otherwise the project builds on Windows and fails on Linux. The build archive is stored with Git LFS |

### `Config/`

| File | Description |
|---|---|
| `DefaultEngine.ini` | Startup map, default classes, rendering settings, gravity, the collision channel for bullets |
| `DefaultGame.ini` | Name, version, authors, maps to package |
| `DefaultInput.ini` | Switches the engine to Enhanced Input, turns mouse smoothing off |
| `DefaultEditor.ini` | Play In Editor settings |

### `Scripts/`

| File | Description |
|---|---|
| `GenerateMaps.py` | Creates or restores both maps and sets their game modes. Only needed if the maps go missing |

### `Content/Maps/`

| File | Description |
|---|---|
| `MainMenu.umap` | Empty main menu map, game mode `FRMenuGameMode` |
| `FiringRange.umap` | Empty range map, game mode `FRRangeGameMode`. Everything in it is built by `FRRangeBuilder` |

### `Release/`

| File | Description |
|---|---|
| `FiringRange-Win64.zip` | The packaged Windows game, stored with Git LFS. It only exists in the GitHub repository — LFS is disabled on Gitea |

### `Source/` — build configuration

| File | Description |
|---|---|
| `FiringRange.Target.cs` | Build rules for the game |
| `FiringRangeEditor.Target.cs` | Build rules for the editor |
| `FiringRange.Build.cs` | Module dependencies, each with a comment explaining why it is needed |
| `FiringRange.h` / `.cpp` | Module entry point and the `LogFiringRange` log category |

### `Source/FiringRange/Core/` — rules and shared data

| File | Description |
|---|---|
| `FRTypes.h` | The vocabulary of the project: ammunition families, hit zones, movement patterns, difficulty, the scoreboard struct |
| `FRVisualUtils` | Loads engine primitives and creates tinted materials |
| `FRSaveGame` | What is stored on disk: settings and the personal best |
| `FRGameInstance` | The only object that survives a level change. Owns the settings |
| `FRRangeGameState` | The session scoreboard: score, accuracy, streak, timer |
| `FRRangeGameMode` | The rules: zone values, restart, difficulty, timed challenge. Builds the range |
| `FRMenuGameMode` | Game mode of the menu map |
| `FRMenuPlayerController` | Shows the main menu and travels to the range map |

### `Source/FiringRange/Player/`

| File | Description |
|---|---|
| `FRCharacter` | The character: movement, camera, Enhanced Input, weapon loadout, ammunition reserve |
| `FRPlayerController` | Input modes, camera pitch limits, pause, starting the challenge |
| `FRHUD` | The whole in-game interface: crosshair, score, accuracy, ammunition, hit markers, timer |

### `Source/FiringRange/Weapons/`

| File | Description |
|---|---|
| `FRWeaponBase` | The weapon state machine: rate of fire, magazine, reload, spread, recoil, ballistics, view model animation |
| `FRPistol` | Semi automatic pistol |
| `FRShotgun` | Shotgun: 8 pellets per shot, shell by shell reload |
| `FRSniperRifle` | Sniper rifle: bolt action, magnifying scope |
| `FRProjectile` | The bullet: a physical projectile under gravity that reports when its flight is over |
| `FRImpactEffect` | Flash and mark at an impact |

### `Source/FiringRange/Targets/`

| File | Description |
|---|---|
| `FRTargetBase` | Board, head, post, three hit zones, knockdown and respawn |
| `FRStationaryTarget` | The fixed targets of the static lanes |
| `FRMovingTarget` | The body of a moving target |
| `FRTargetAIController` | The brain: four movement patterns |
| `FRPatrolPath` | Spline patrol route |

### `Source/FiringRange/Pickups/`

| File | Description |
|---|---|
| `FRAmmoPickup` | Ammunition crates: collected on contact, back after a delay |

### `Source/FiringRange/Level/`

| File | Description |
|---|---|
| `FRRangeBuilder` | Builds the range: ground, berms, canopy, dividers, targets, crates, lighting, player start |

### `Source/FiringRange/UI/`

| File | Description |
|---|---|
| `FRUIStyle` | Colours, fonts, button and slider styles |
| `SFRMainMenu` | Main menu: start, settings, quit, personal best |
| `SFRSettingsPanel` | Mouse sensitivity, inverted look, difficulty. Shared by the menu and the pause screen |
| `SFRPauseMenu` | Resume, settings, restart, back to the main menu |

---

## What makes this project unusual

**There is not a single authored asset in this repository.** The only binary
files are two empty maps of 9 KB each.

The brief allows Blueprints or C++. This project uses C++ throughout and goes one
step further: it has no authored content at all — no meshes, textures,
materials, sounds, animations, UMG widgets, input assets, or level geometry.

| Normally an asset | Here instead |
|---|---|
| Weapon and target meshes | Primitives from `/Engine/BasicShapes`, assembled and tinted in code |
| Materials | Dynamic instances of the engine `BasicShapeMaterial` |
| `InputAction`, `InputMappingContext` | Objects created with `NewObject` at runtime |
| UMG menus | Slate widgets written in C++ |
| HUD widgets | Canvas drawing in `AHUD::DrawHUD` |
| Reload and recoil animations | Procedural transform animation of the weapon |
| Level geometry, lighting, player start | `AFRRangeBuilder`, which builds everything when the session starts |

The reason is practical rather than stylistic: a repository of plain text can be
read, reviewed line by line, merged and diffed. A `.uasset` cannot. The trade
off is that the range looks like blocks, and that trade is deliberate.

The two `.umap` files are the only exception. A map is the one thing the engine
cannot create from code at load time, so it has to exist as a file — but both
maps are **empty**, holding only a reference to their game mode, and everything
else appears at startup.

---

## How it works

```
             UFRGameInstance                  survives level changes
             (settings, record)               ──────────────────────
                     │
        ┌────────────┴─────────────┐
        │                          │
  AFRMenuGameMode           AFRRangeGameMode ─── AFRRangeBuilder
  (MainMenu map)            (FiringRange map)    (builds the range)
        │                          │
  AFRMenuPlayerController    AFRRangeGameState ──────────► AFRHUD
  → SFRMainMenu              (scoreboard)                  (reads only)
      → SFRSettingsPanel            ▲
                                    │ writes
                          ┌─────────┴──────────┐
                          │                    │
                  AFRWeaponBase          AFRTargetBase
                  (shots fired)          (hits, zones)
                          │                    ▲
                    AFRProjectile ─────────────┘
                    (damage, end of flight)
```

One shot travels through the project like this:

1. `AFRCharacter` receives the fire action and calls `StartFire` on the weapon.
2. `AFRWeaponBase` checks the rate of fire and the magazine, spends a round,
   traces forward from the camera to find what the crosshair covers, and
   launches the projectiles at that point — aimed slightly high, by exactly the
   distance the bullet will fall on the way.
3. The weapon reports how many projectiles left the muzzle. The game mode adds
   them to the denominator of the accuracy figure.
4. `AFRProjectile` flies and, on impact, applies damage through the engine's
   standard damage pipeline.
5. `AFRTargetBase::TakeDamage` works out which zone was struck, announces the
   hit, falls over and schedules its own return.
6. `AFRRangeGameMode` turns the zone and the range into points and writes them
   to `AFRRangeGameState`.
7. `AFRHUD`, subscribed to the scoreboard, draws a hit marker.

At no point does the bullet know what a target is, the target know what a score
is, or the HUD know what a weapon is.

### Design decisions worth knowing about

**A truthful crosshair and real ballistics at the same time.** The brief asks
for a crosshair that marks the exact point of impact *and* for bullets that obey
physics. Those pull in opposite directions: a bullet under gravity falls below
where the crosshair points. `AFRWeaponBase::ComputeLaunchVelocity` resolves it —
it works out the flight time from the distance and the muzzle speed, works out
the drop over that time, and aims the barrel exactly that far higher. The bullet
arcs and still lands on the dot.

**The crosshair opens when the weapon is less accurate.** The gap between its
arms is the spread cone projected into pixels. It widens while running and
closes while aiming. A crosshair of constant size would be lying.

**Recoil is written into the control rotation in degrees**, not fed through
`AddControllerPitchInput`. Input goes through the mouse sensitivity setting, so
a player on high sensitivity would otherwise get a completely different weapon.

**Bullets have their own collision channel.** A shotgun releases eight pellets
from the same point on the same frame. On a shared channel they would block each
other at spawn.

**Targets are pawns.** Only a pawn can be possessed by an `AAIController`, and
the moving targets need one.

**The target AI does not use navigation.** A navigation mesh is data baked into
a level asset, and this project has no level asset to bake it into. Following a
spline analytically needs no data and gives exact, repeatable motion — which is
what a training range wants anyway.

**The bullet closes the shot, the target scores it.** When the trigger is
pulled, nobody knows yet whether the shot will hit: the bullet is still in the
air. So `AFRProjectile` reports when its flight is over, including when it simply
runs out of time. Whether it hit is said only by the target, through its own
event. The damage result cannot be used for that: in Unreal every actor accepts
damage by default, and a wall "accepts" a bullet just as a bullseye does.

**Cross platform from the start.** Line endings in the repository are always LF
(`.gitattributes`), `#include` paths are fully qualified and in their exact case
for the case sensitive file systems of Linux and macOS, quitting uses
`UKismetSystemLibrary::QuitGame` rather than a platform call, and the code
contains no absolute paths.

**The range is outdoors.** A sun, a sky atmosphere and a real time capture sky
light give correct lighting without a single imported file.

---

## Assignment checklist

| Requirement | Where it lives |
|---|---|
| Main menu on a separate map | `MainMenu` map, `AFRMenuGameMode`, `SFRMainMenu` |
| Start Game button | `SFRMainMenu` → `AFRMenuPlayerController::HandleStartGame` |
| Mouse sensitivity setting | `SFRSettingsPanel`, persisted by `UFRGameInstance` |
| Quit button | `AFRMenuPlayerController::HandleQuitGame` |
| Accurate crosshair | `AFRHUD::DrawCrosshair` + `AFRWeaponBase::ComputeLaunchVelocity` |
| Accuracy readout | `FFRRangeStats::GetAccuracy`, drawn by `AFRHUD::DrawScorePanel` |
| Ammunition and reload status | `AFRHUD::DrawWeaponPanel` |
| Player movement | `AFRCharacter`, Enhanced Input |
| Aim, fire and reload input | `AFRCharacter::BuildInputActions` |
| Ammunition pickups | `AFRAmmoPickup` |
| Recoil | `AFRWeaponBase::ApplyRecoil` and `UpdateRecoil` |
| Reload animation | `AFRWeaponBase::UpdateViewModel`, procedural |
| Projectile physics | `AFRProjectile` |
| Hit detection with feedback | `AFRProjectile::HandleHit`, `AFRImpactEffect` |
| Stationary target | `AFRStationaryTarget` |
| Moving target with AI | `AFRMovingTarget` + `AFRTargetAIController` |
| Target hit detection | `AFRTargetBase::TakeDamage`, `ResolveHitZone` |
| Targets respawn on a timer | `AFRTargetBase::HandleKnockedDown` |
| Coherent theme and lighting | `AFRRangeBuilder::BuildLighting` and the palette next to it |
| Stationary section | `AFRRangeBuilder::BuildStationarySection` |
| Moving section | `AFRRangeBuilder::BuildMovingSection` |
| Starts with a loaded weapon | `AFRWeaponBase::BeginPlay` |
| Pause menu | `AFRPlayerController::OpenPauseMenu`, `SFRPauseMenu` |
| Restart from the pause menu | `AFRRangeGameMode::RestartRange` |
| Back to the main menu | `AFRPlayerController::HandleQuitToMainMenu` |
| **Bonus** — several weapons | `AFRPistol`, `AFRShotgun`, `AFRSniperRifle` |
| **Bonus** — advanced target AI | `EFRTargetMotion`, four patterns, three difficulty levels |
| **Bonus** — hit zones and headshots | `AFRTargetBase::ResolveHitZone`, scored in `AFRRangeGameMode` |
| **Bonus** — timed challenge | `AFRRangeGameMode::StartTimedChallenge`, key `T` |
