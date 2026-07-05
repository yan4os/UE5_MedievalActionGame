# Medieval Action-RPG (Unreal Engine 5, C++)

A third-person **medieval action-RPG** built from the ground up in **Unreal Engine 5.6** with **C++** as the primary gameplay language. The project implements a full vertical slice of an action-RPG loop: exploration across multiple levels, real-time melee combat against AI-driven enemies, a stamina-gated sprint system, collectible loot and weapons, environmental puzzles, and a complete save / load system.

The codebase is written to production conventions used in commercial Unreal projects — a clean split between C++ gameplay logic and Blueprint-exposed data, heavy use of `UPROPERTY`/`UFUNCTION` reflection, component-based composition, and the Gameplay Framework (`ACharacter`, `AController`, `AGameModeBase`, `USaveGame`).

---

## Overview

You play as a sword-wielding hero exploring a medieval world. Enemies patrol the environment and engage the moment you enter their aggro range; combat is driven by animation montages with collision-gated damage windows. Health and stamina are managed in real time, coins and health pickups are scattered throughout the levels, doors are opened via floor switches, and progress can be persisted to disk and restored on demand — including the player's equipped weapon, stats, and world position.

**Engine:** Unreal Engine 5.6 · **Language:** C++ (with Blueprint integration) · **Genre:** Third-Person Action-RPG · **Platform:** Windows (PC)

---

## Implemented Features

### Player Character (`AMain`)
- **Third-person camera rig** using a `USpringArmComponent` + `UCameraComponent` with configurable turn / look-up rates (mouse and gamepad).
- **State-machine driven movement** via two custom enums — `EMovementStatus` (Normal / Sprinting / Dead) and `EStaminaStatus` (Normal / BelowMinimum / Exhausted / ExhaustedRecovering).
- **Stamina-gated sprint system**: holding sprint drains stamina at a configurable rate; dropping below a minimum threshold forces a recovery state before sprinting is available again — preventing sprint-spam.
- **RPG stat system**: `Health` / `MaxHealth`, `Stamina` / `MaxStamina`, and a `Coins` currency counter, all Blueprint-exposed for HUD binding.
- **Damage & death handling** through an override of the engine's `TakeDamage()` pipeline, with death animations and a `DeathEnd()` montage callback.
- **Combat targeting**: automatically acquires the nearest valid enemy, rotates smoothly toward it during attacks via interpolated yaw (`GetLookAtRotationYaw`), and drives an on-screen enemy health bar.

### Combat System
- **Weapon system** (`AWeapon` → `AItem`): equippable weapons with `EWeaponState` (Pickup / Equipped), socket attachment to the character skeleton, equip sounds, optional idle particle FX, and a floating/bobbing idle animation while un-equipped.
- **Collision-gated melee damage**: weapons carry a `UBoxComponent` combat hitbox that is enabled only during the active frames of an attack montage (`EnableCollision` / `DeactivateCollision` are called from anim notifies), so damage lands exactly on the swing.
- **Damage-type routing** using `TSubclassOf<UDamageType>` and a stored `WeaponInstigator` controller, so kills are correctly attributed.
- **Hit feedback**: impact particle systems and Sound Cues on both the player and enemies.

### Enemy AI (`AEnemy`)
- **Perception via overlap spheres**: an **Aggro Sphere** detects the player and begins pursuit; a tighter **Combat Sphere** triggers attacks.
- **`AAIController` navigation** — enemies path toward the player (`MoveToTarget`) using the **AIModule** / navmesh.
- **AI state machine** (`EEnemyMovementStatus`: Idle / MoveToTarget / Attacking / Dead).
- **Randomized attack cadence** using timers between `AttackMinTime` and `AttackMaxTime` for non-robotic pacing.
- **Full combat parity with the player**: enemies have their own health, damage, combat collision, montages, `TakeDamage` handling, a death delay, and a `Disappear()` cleanup pass.

### World Interaction & Gameplay Actors
- **Item base class** (`AItem`): sphere-collision pickups with overlap particles, sounds, and optional idle rotation — the shared parent for weapons, pickups, and explosives.
- **Pickups** (`APickup`): coins / health pickups that raise a `BlueprintImplementableEvent` so designers can extend behavior in Blueprint.
- **Explosives** (`AExplosive`): damage-dealing hazards that apply damage on overlap.
- **Floor switches** (`AFloorSwitch`): pressure-plate puzzles that raise / lower doors, with a timed auto-close and Blueprint-implementable animation events.
- **Spawn volumes** (`ASpawnVolume`): randomly spawn one of several actor classes at a random point inside a box volume — used to populate enemies / critters.
- **Level transition volumes** (`ALevelTransitionVolume`): trigger seamless travel between maps by name.
- **Floating platforms & floaters**: sine-driven moving-platform actors for traversal.
- **Critter pawn** (`ACritter`): a lightweight standalone `APawn` with its own movement and camera — a self-contained sandbox example.

### HUD & UI (`AMainPlayerController`)
- **UMG HUD overlay** spawned and owned by the player controller (health / stamina / coins bars).
- **Dynamic enemy health bar** that is shown / hidden based on combat state and projected to the enemy's screen position each frame.

### Save / Load System (`USaveMyGame`)
- Custom `USaveGame` subclass persisting a `FCharacterStats` struct: health, max health, stamina, max stamina, coins, world **location** and **rotation**, and the **equipped weapon by name**.
- **Weapon restoration** on load via an `AItemStorage` data asset that maps saved weapon names back to spawnable weapon classes — so the player reloads fully equipped.
- Save / Load are Blueprint-callable, allowing bind-to-menu functionality.

### Custom Movement Prototype (`ACollider` + `UColliderMovementComponent`)
- A from-scratch pawn built on a `USphereComponent` root with a **custom `UPawnMovementComponent`** — demonstrating manual velocity integration and movement handling independent of `UCharacterMovementComponent`.

---

## Architecture & Technical Highlights

| Area | Details |
|------|---------|
| **Language split** | Core gameplay, combat, AI, and persistence are all authored in **C++**; Blueprint is used for data, VFX wiring, and designer-facing events via `BlueprintImplementableEvent` / `BlueprintCallable`. |
| **Composition** | Actors are assembled from engine components (spring arm, cameras, sphere/box collision, skeletal meshes, particle systems) rather than deep inheritance. |
| **Reflection & tooling** | Extensive `UPROPERTY` metadata (`EditAnywhere`, `BlueprintReadWrite`, categories) exposes tunable parameters to designers directly in the editor. |
| **Gameplay Framework** | Uses `ACharacter`, `AAIController`, `APlayerController`, `AGameModeBase`, `USaveGame`, and the engine `TakeDamage` / `UDamageType` pipeline. |
| **Enhanced Input ready** | Configured with the **Enhanced Input** subsystem (`EnhancedPlayerInput` / `EnhancedInputComponent`) alongside classic action/axis mappings. |
| **Animation** | Custom `UAnimInstance` subclasses (`MainAnimInstance`, `EnemyAnimInstance`) drive locomotion and combat blends; anim notifies gate combat collision and sound cues. |
| **Modules** | Depends on `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `UMG`, `AIModule`, `Slate`, `SlateCore`. |

### Project Structure
```
TheGame.uproject              # UE 5.6 project descriptor
Config/                       # Engine, Input, Game & Editor config (input mappings, etc.)
Source/TheGame/
├── Main / MainAnimInstance / MainPlayerController   # Player, animation, HUD controller
├── Enemy / EnemyAnimInstance                        # AI enemy + animation
├── Weapon / Item / Pickup / Explosive / ItemStorage # Items & combat gear
├── FloorSwitch / SpawnVolume / LevelTransitionVolume# World interaction
├── FloatinngPlatform / GamePlayActors/Floater       # Moving platforms
├── Critter                                          # Standalone example pawn
├── Collider / ColliderMovementComponent             # Custom movement component
├── SaveMyGame                                       # Save/Load system
└── TheGameModeBase                                  # Game mode
```

---

## Controls

| Input | Action |
|-------|--------|
| **W / A / S / D** | Move |
| **Mouse** | Look |
| **Left Shift** | Sprint (stamina-gated) |
| **Space** | Jump |
| **Left Mouse Button** | Attack |
| **Gamepad** | Fully mapped (left stick move, right stick look, face buttons for jump/attack, shoulder to sprint) |

---

## Getting Started

### Requirements
- **Unreal Engine 5.6**
- **Visual Studio 2022** with the *Game Development with C++* workload
- Windows 10 / 11

### Build & Run
1. Clone the repository.
2. Right-click `TheGame.uproject` → **Generate Visual Studio project files**.
3. Open the generated `.sln` and build the **Development Editor | Win64** configuration.
4. Launch by opening `TheGame.uproject`, or run from Visual Studio.

> **Note:** This repository contains the **source, configuration, and project descriptor**. Binary content (`Content/`), build intermediates, and derived data are excluded via `.gitignore`, as is standard for Unreal source repositories.

---

## Strengths & What This Project Demonstrates

- **Gameplay C++ proficiency** — combat, AI, state machines, and persistence written directly against the Unreal Gameplay Framework, not just Blueprint scripting.
- **Systems thinking** — interlocking systems (stamina ↔ sprint, aggro ↔ combat spheres ↔ AI states, save ↔ weapon storage) rather than isolated features.
- **Correct combat fundamentals** — animation-notify-gated hitboxes and the engine damage pipeline, the same pattern used in shipping action games.
- **Designer-friendly code** — nearly every tunable value is `UPROPERTY`-exposed and categorized for editor iteration without recompiling.
- **Clean C++/Blueprint boundary** — logic in C++, content and event hooks in Blueprint, communicating through a well-defined reflected API.
- **Full feature loop** — a genuinely *playable* slice with a start, combat, loot, puzzles, level transitions, and save/load, not a collection of disconnected demos.

---

## 🛠️ Tech Stack

**Unreal Engine 5.6** · **C++** · **Blueprints** · **Enhanced Input** · **UMG** · **AI Module (Navmesh + AIController)** · **Animation Blueprints & Montages** · **Niagara/Cascade Particles** · **SaveGame Serialization**

---

## Notes

This project began as a structured study of Unreal Engine C++ gameplay programming and grew into a self-contained action-RPG vertical slice. It is intended as a **portfolio piece** showcasing engine-level gameplay engineering in C++.

---

<p align="center"><i>Built with Unreal Engine 5 &amp; C++</i></p>
