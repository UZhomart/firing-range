# Firing Range

> **This is an educational project.** It was written as a learning exercise for the
> Firing Range assignment and is not a commercial product. Everything in this
> repository exists to demonstrate how the systems of a first person shooter fit
> together, and the code is commented with that goal in mind rather than for
> brevity.

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

A first person marksmanship range: a weapon system with three firearms, physics
driven bullets, stationary and AI driven moving targets, ammunition pickups, a
head up display that reports accuracy, a main menu on its own map, a pause menu
and a timed challenge mode.

---

## Contents

1. [What makes this project unusual](#what-makes-this-project-unusual)
2. [Getting it running](#getting-it-running)
3. [Controls](#controls)
4. [Repository structure](#repository-structure)
5. [How the systems fit together](#how-the-systems-fit-together)
6. [Design decisions worth knowing about](#design-decisions-worth-knowing-about)
7. [Assignment checklist](#assignment-checklist)

---

## What makes this project unusual

**There is not a single binary asset in this repository.**

The brief allows either Blueprints or C++. This project takes C++ all the way,
and then goes one step further: it also avoids every kind of authored content.
No meshes, no textures, no materials, no sounds, no animations, no UMG widgets,
no input assets, and no level geometry are committed.

Everything is built at runtime from what ships inside the engine:

| Normally an asset | Here instead |
|---|---|
| Weapon and target meshes | Primitives from `/Engine/BasicShapes`, assembled and tinted in code |
| Materials | Dynamic instances of the engine `BasicShapeMaterial` |
| Input actions and mapping contexts | `UInputAction` and `UInputMappingContext` objects built with `NewObject` |
| UMG menus | Slate widgets written in C++ |
| HUD widgets | Canvas drawing in `AHUD::DrawHUD` |
| Reload and recoil animations | Procedural transform animation of the weapon |
| Level geometry, lighting, player start | `AFRRangeBuilder`, which spawns the whole range when the session starts |

The reason is practical rather than stylistic: a repository of plain text can be
read, reviewed line by line, merged and diffed. A `.uasset` cannot. The trade
off is that the range looks like blocks, and that trade is deliberate.

The two `.umap` files are the only exception. They are the one thing the engine
will not create from code at load time, and they are **empty** - see the setup
step below.

---

## Getting it running

### Downloads

| What | Link |
|---|---|
| Visual Studio 2022 Community | https://visualstudio.microsoft.com/vs/community/ |
| Epic Games account | https://www.epicgames.com/id/register |
| Unreal Engine 5.5 | https://www.unrealengine.com/en-US/download |
| Git LFS (optional) | https://git-lfs.com |

About 63 GB of free disk space is needed for the engine, the toolchain and the
build artefacts.

### Requirements

- Unreal Engine 5.5
- A C++ toolchain for your platform:
  - **Windows** - Visual Studio 2022 with *Desktop development with C++* and the
    *Game development with C++* workload
  - **Linux** - clang, as installed by the engine's `Setup.sh`
  - **macOS** - Xcode with the command line tools

### Steps

1. **Clone the repository.**

   ```
   git clone <repository-url> firing-range
   cd firing-range
   ```

2. **Generate the project files and build.**

   Right click `FiringRange.uproject` and choose *Generate Visual Studio project
   files* on Windows, or run `GenerateProjectFiles` from your engine
   installation on Linux and macOS. Then build the `FiringRangeEditor` target.

   Opening `FiringRange.uproject` directly also works: the editor offers to
   build the missing module and does it for you.

3. **Create the two maps.** They are not in the repository because a `.umap` is
   a binary asset. They are empty, so this takes four clicks:

   - *File > New Level > Empty Level*, save as `Content/Maps/MainMenu`
   - *File > New Level > Empty Level*, save as `Content/Maps/FiringRange`

   Or let the editor do it: open *Window > Output Log*, switch the console at
   the bottom to **Python**, and run

   ```
   exec(open(r"<project>/Scripts/GenerateMaps.py").read())
   ```

4. **Press Play.** The game boots into `MainMenu`, because
   `Config/DefaultEngine.ini` names it as the default map and
   `AFRMenuGameMode` as the default game mode. *Start Game* travels to the range
   map, where `AFRRangeGameMode` builds the entire range before the player is
   given a pawn.

There is nothing to assign in the editor. No Blueprint has to be created, no
class has to be picked in a dropdown, and no actor has to be dragged into a
level.

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
| `Esc` | Pause menu |

A gamepad is mapped throughout, on the same actions.

Walk over an ammunition crate to restock. A crate is only consumed when the
rounds actually fit, so walking over one while full leaves it standing.

---

## Repository structure

```
firing-range/
├── FiringRange.uproject
├── README.md
├── firing-range__ts.md
├── .gitignore
├── .gitattributes
├── Config/
│   ├── DefaultEngine.ini
│   ├── DefaultGame.ini
│   ├── DefaultInput.ini
│   └── DefaultEditor.ini
├── Scripts/
│   └── GenerateMaps.py
├── resources/
└── Source/
    ├── FiringRange.Target.cs
    ├── FiringRangeEditor.Target.cs
    └── FiringRange/
        ├── FiringRange.Build.cs
        ├── FiringRange.h / .cpp
        ├── Core/
        ├── Player/
        ├── Weapons/
        ├── Targets/
        ├── Pickups/
        ├── Level/
        └── UI/
```

### Every item, and why it is there

#### Root

| Item | What it is |
|---|---|
| `FiringRange.uproject` | Project manifest. Names the engine version (5.5), the single runtime module, and the two plugins the project turns on: Enhanced Input, and the Python script plugin used only by the map generator. |
| `README.md` | This file. |
| `firing-range__ts.md` | The original assignment brief, kept under version control so the result can be checked against what was asked for. |
| `.gitignore` | Keeps the generated half of an Unreal project out of the repository: `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, IDE files. It also excludes `learn/`, which holds the authors' personal study notes and is not part of the deliverable. |
| `.gitattributes` | Forces LF line endings inside the repository. Without it a file written on Windows arrives on Linux with stray carriage returns, which is the classic reason a project builds on one machine and not on another. |

#### `Config/`

| Item | What it is |
|---|---|
| `DefaultEngine.ini` | Read by the engine before any of our code runs. Names the boot map and the default game mode and game instance, turns off motion blur and auto exposure because both interfere with aiming, sets gravity, and declares the custom collision channel bullets use. |
| `DefaultGame.ini` | Project name, version, authors, and which maps get cooked into a packaged build. |
| `DefaultInput.ini` | Switches the player input and input component classes over to Enhanced Input. Without these two lines every input binding in the project silently does nothing. It also turns off mouse smoothing and acceleration, so the sensitivity slider maps one to one onto view rotation. |
| `DefaultEditor.ini` | Editor only conveniences: Play In Editor runs as a single standalone window, which is how the packaged game behaves. |

#### `Scripts/`

| Item | What it is |
|---|---|
| `GenerateMaps.py` | Editor script that creates the two empty maps and sets their game mode override. Optional - the same thing takes four clicks by hand. |

#### `Source/` - build configuration

| Item | What it is |
|---|---|
| `FiringRange.Target.cs` | Build rules for the packaged game. |
| `FiringRangeEditor.Target.cs` | Build rules for the editor. |
| `FiringRange.Build.cs` | Module dependencies, one per line with a comment saying why each is needed. |
| `FiringRange.h` / `.cpp` | Module entry point and the `LogFiringRange` log category every subsystem writes to. |

#### `Source/FiringRange/Core/` - rules and shared data

| Item | What it is |
|---|---|
| `FRTypes.h` | The vocabulary of the project: ammunition families, hit zones, movement patterns, difficulty levels, session states, and the `FFRRangeStats` scoreboard struct. Everything else speaks in these terms. |
| `FRVisualUtils.h` / `.cpp` | Loads engine primitives and builds tinted dynamic materials. The single place that knows the project has no art. |
| `FRSaveGame.h` / `.cpp` | Persistent settings and records on disk. |
| `FRGameInstance.h` / `.cpp` | The only object that survives travelling between maps, so it owns the settings and the personal best. Broadcasts when a setting changes so listeners apply it immediately. |
| `FRRangeGameState.h` / `.cpp` | The scoreboard. The game mode writes it, the HUD reads it, and neither talks to the other. |
| `FRRangeGameMode.h` / `.cpp` | The rules: what a hit zone is worth, when a target respawns, what a restart does, and the timed challenge clock. Also builds the range during `InitGame`, before the player exists. |
| `FRMenuGameMode.h` / `.cpp` | Game mode of the menu map. No weapons, no scoring, no HUD. |
| `FRMenuPlayerController.h` / `.cpp` | Puts the main menu in the viewport and answers the two entries that leave it. |

#### `Source/FiringRange/Player/`

| Item | What it is |
|---|---|
| `FRCharacter.h` / `.cpp` | The player: movement, camera, the whole Enhanced Input setup built in code, the weapon loadout, aiming, and the ammunition reserve. |
| `FRPlayerController.h` / `.cpp` | What outlives the pawn: input modes, camera pitch limits, the pause state machine and the challenge key. |
| `FRHUD.h` / `.cpp` | Everything on screen during play, drawn on the canvas: the dynamic crosshair, score and accuracy, magazine and reserve, reload progress, hit markers and the challenge countdown. |

#### `Source/FiringRange/Weapons/`

| Item | What it is |
|---|---|
| `FRWeaponBase.h` / `.cpp` | The weapon state machine: rate of fire, magazine, reload, spread, recoil, the ballistic aiming solution and the procedural view model animation. The three firearms share all of it. |
| `FRPistol.h` / `.cpp` | Semi automatic sidearm. The reference the others are tuned against. |
| `FRShotgun.h` / `.cpp` | Eight pellets per trigger pull and a shell by shell reload that firing can interrupt. |
| `FRSniperRifle.h` / `.cpp` | Bolt action, a magnifying scope, and a reticle of its own on the HUD. |
| `FRProjectile.h` / `.cpp` | The bullet: a swept sphere under `UProjectileMovementComponent`, pulled down by gravity, which reports to the game mode whether it scored. |
| `FRImpactEffect.h` / `.cpp` | The flash and scorch mark at an impact, animated and destroyed in under half a second. |

#### `Source/FiringRange/Targets/`

| Item | What it is |
|---|---|
| `FRTargetBase.h` / `.cpp` | Board, head and post, three scoring zones, the knockdown animation and the respawn timer. A pawn rather than an actor, because a controller can only possess a pawn. |
| `FRStationaryTarget.h` / `.cpp` | The fixed boards of the static lanes. |
| `FRMovingTarget.h` / `.cpp` | The body of a moving target: a pawn with a floating movement component and no decisions of its own. |
| `FRTargetAIController.h` / `.cpp` | The brain: four movement patterns, from following a route to unpredictable strafing. |
| `FRPatrolPath.h` / `.cpp` | A spline that defines where a moving target may go. |

#### `Source/FiringRange/Pickups/`

| Item | What it is |
|---|---|
| `FRAmmoPickup.h` / `.cpp` | The ammunition crates: collected on overlap, only when the rounds fit, and back after a delay. |

#### `Source/FiringRange/Level/`

| Item | What it is |
|---|---|
| `FRRangeBuilder.h` / `.cpp` | Builds the range: ground, berms, firing line and canopy, lane dividers, distance markers, both target sections, the crates, the lighting and the player start. |

#### `Source/FiringRange/UI/`

| Item | What it is |
|---|---|
| `FRUIStyle.h` / `.cpp` | Colours, fonts, brushes and the shared button and slider styles. Built from solid colour brushes and the engine's own Roboto, so no style asset is needed. |
| `SFRMainMenu.h` / `.cpp` | The front end: start, settings, quit, and the personal best line. |
| `SFRSettingsPanel.h` / `.cpp` | Mouse sensitivity, aim sensitivity, inverted look and difficulty. Used by both the main menu and the pause menu, which is why the two always agree. |
| `SFRPauseMenu.h` / `.cpp` | Resume, settings, restart and return to the main menu, over a dimmed view of the frozen range. |

---

## How the systems fit together

```
             UFRGameInstance                  survives level travel
             (settings, records)              ─────────────────────
                     │
        ┌────────────┴─────────────┐
        │                          │
  AFRMenuGameMode           AFRRangeGameMode ─── AFRRangeBuilder
  (MainMenu map)            (FiringRange map)    (builds everything)
        │                          │
  AFRMenuPlayerController    AFRRangeGameState ──────────► AFRHUD
  → SFRMainMenu              (the scoreboard)              (reads only)
      → SFRSettingsPanel            ▲
                                    │ writes
                          ┌─────────┴──────────┐
                          │                    │
                  AFRWeaponBase          AFRTargetBase
                  (shots fired)          (hits, zones)
                          │                    ▲
                    AFRProjectile ─────────────┘
                    (damage, outcome)
```

One trigger pull travels through the project like this:

1. `AFRCharacter` receives the fire action and calls `StartFire` on the weapon.
2. `AFRWeaponBase` checks the rate of fire and the magazine, spends a round,
   traces forward from the camera to find the point the crosshair covers, and
   launches one projectile per pellet towards it - aimed slightly high, by
   exactly the distance the bullet will fall on the way.
3. It announces how many projectiles left the muzzle. The game mode adds them to
   the denominator of the accuracy figure.
4. `AFRProjectile` flies, and on impact applies point damage through the engine
   damage pipeline.
5. `AFRTargetBase::TakeDamage` works out which zone was struck, announces the
   hit, falls over and schedules its own return.
6. `AFRRangeGameMode` turns the zone and the range into points and writes them to
   `AFRRangeGameState`.
7. `AFRHUD` was listening to the game state, and draws a hit marker.

At no point does the bullet know what a target is, the target know what a score
is, or the HUD know what a weapon is.

---

## Design decisions worth knowing about

**A truthful crosshair and real ballistics at the same time.** The brief asks
for a crosshair that marks the exact point of impact *and* for bullets that obey
physics. Those pull in opposite directions, because a bullet with gravity falls
below where it was pointed. `AFRWeaponBase::ComputeLaunchVelocity` resolves it:
it works out the flight time from the distance and the muzzle speed, works out
the drop over that time, and aims the muzzle that far above the crosshair. The
bullet arcs, and it still lands on the dot.

**The crosshair opens when the weapon is less accurate.** Its gap is the current
spread cone projected into pixels, so it widens while running and closes while
aiming. A crosshair that stayed the same size would be lying.

**Recoil is written into the control rotation in degrees**, not fed through
`AddControllerPitchInput`. Input goes through the mouse sensitivity setting, so
a player on high sensitivity would otherwise get a completely different weapon.

**Bullets have their own collision channel.** A shotgun releases eight pellets
from the same muzzle point on the same frame. On any shared channel they would
block each other at spawn. The `Projectile` channel declared in
`DefaultEngine.ini` lets bullets ignore bullets and nothing else.

**Targets are pawns.** Only a pawn can be possessed by an `AAIController`, and
the moving targets need one.

**The target AI never queries navigation.** A navigation mesh is data baked into
a level asset, and this project has no level asset to bake it into. The
controller follows its spline analytically instead, which needs no navigation
data and gives exact, repeatable motion - which is what a training range wants
anyway.

**A miss is reported by the bullet, not by the trigger.** Whether a shot missed
is unknowable at the moment it is fired, because the bullet is still travelling.
`AFRProjectile` reports its own outcome when it resolves, including when it
simply runs out of flight time.

**The range is outdoors.** A sun, a sky atmosphere and a real time capture sky
light give correct lighting with no imported content. An indoor room would have
needed light fixtures and a captured cubemap.

**Cross platform from the start.** LF line endings enforced by
`.gitattributes`, fully qualified include paths so case sensitive file systems
are happy, `UKismetSystemLibrary::QuitGame` instead of a platform call, and no
absolute paths anywhere.

---

## Assignment checklist

| Requirement | Where it lives |
|---|---|
| Main menu on a separate map | `MainMenu` map, `AFRMenuGameMode`, `SFRMainMenu` |
| Start Game button | `SFRMainMenu` → `AFRMenuPlayerController::HandleStartGame` |
| Settings with mouse sensitivity | `SFRSettingsPanel`, persisted by `UFRGameInstance` |
| Quit button | `AFRMenuPlayerController::HandleQuitGame` |
| Accurate crosshair | `AFRHUD::DrawCrosshair` with `AFRWeaponBase::ComputeLaunchVelocity` |
| Accuracy readout | `FFRRangeStats::GetAccuracy`, drawn by `AFRHUD::DrawScorePanel` |
| Ammunition and reload status | `AFRHUD::DrawWeaponPanel` |
| Player movement | `AFRCharacter`, Enhanced Input |
| Aim, shoot, reload input | `AFRCharacter::BuildInputActions` |
| Ammo pickups | `AFRAmmoPickup` |
| Recoil | `AFRWeaponBase::ApplyRecoil` and `UpdateRecoil` |
| Reload animation | `AFRWeaponBase::UpdateViewModel`, procedural |
| Projectile physics | `AFRProjectile` |
| Hit detection with feedback | `AFRProjectile::HandleHit`, `AFRImpactEffect` |
| Stationary target | `AFRStationaryTarget` |
| Moving target with AI | `AFRMovingTarget` and `AFRTargetAIController` |
| Target hit detection | `AFRTargetBase::TakeDamage` and `ResolveHitZone` |
| Targets respawn on a timer | `AFRTargetBase::HandleKnockedDown` |
| Coherent theme and lighting | `AFRRangeBuilder::BuildLighting` and the palette above it |
| Stationary section | `AFRRangeBuilder::BuildStationarySection` |
| Moving section | `AFRRangeBuilder::BuildMovingSection` |
| Starts with a loaded weapon | `AFRWeaponBase::BeginPlay` |
| Pause menu | `AFRPlayerController::OpenPauseMenu`, `SFRPauseMenu` |
| Restart from the pause menu | `AFRRangeGameMode::RestartRange` |
| Return to the main menu | `AFRPlayerController::HandleQuitToMainMenu` |
| **Bonus** - several weapons | `AFRPistol`, `AFRShotgun`, `AFRSniperRifle` |
| **Bonus** - advanced target AI | `EFRTargetMotion`, four patterns, three difficulty levels |
| **Bonus** - hit zones and headshots | `AFRTargetBase::ResolveHitZone`, scored in `AFRRangeGameMode` |
| **Bonus** - timed challenge | `AFRRangeGameMode::StartTimedChallenge`, key `T` |
