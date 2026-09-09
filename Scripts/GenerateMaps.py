# Copyright zutemiss & dshadykh. Educational project.
#
# Creates the two maps the project needs.
#
# A .umap is a binary asset and this repository holds source only, so the maps
# are not committed. They are also the only two files the project cannot express
# in code - everything inside them is built at runtime by AFRRangeBuilder, so
# both maps stay completely empty.
#
# Run it once after opening the project for the first time:
#
#     Window > Output Log, switch the console mode to Python, then
#     exec(open(r"<project>/Scripts/GenerateMaps.py").read())
#
# Or from a terminal, without opening the editor by hand:
#
#     UnrealEditor-Cmd "<project>/FiringRange.uproject" -run=pythonscript \
#         -script="<project>/Scripts/GenerateMaps.py"
#
# Creating the maps by hand works just as well. See the README for the four
# clicks it takes.

import unreal

MAPS = [
    ("/Game/Maps/MainMenu", "/Script/FiringRange.FRMenuGameMode"),
    ("/Game/Maps/FiringRange", "/Script/FiringRange.FRRangeGameMode"),
]


def log(message):
    unreal.log("[FiringRange] {0}".format(message))


def set_game_mode_override(game_mode_path):
    """Points the World Settings of the current level at one of our game modes.

    Without this the map would fall back to the global default, which is the menu
    game mode, and opening the range map directly in the editor would show a menu
    instead of a range. The game itself does not depend on it: the menu passes the
    game mode in the travel URL as well.
    """
    try:
        world = unreal.EditorLevelLibrary.get_editor_world()
        settings = unreal.GameplayStatics.get_actor_of_class(world, unreal.WorldSettings)

        if settings is None:
            log("World Settings not found, set the game mode override by hand.")
            return

        game_mode_class = unreal.load_class(None, game_mode_path)
        if game_mode_class is None:
            log("Could not load {0}. Compile the C++ module first.".format(game_mode_path))
            return

        settings.set_editor_property("default_game_mode", game_mode_class)
        log("Game mode override set to {0}.".format(game_mode_path))

    except Exception as error:
        log("Could not set the game mode override: {0}".format(error))


def create_map(package_path, game_mode_path):
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

    if unreal.EditorAssetLibrary.does_asset_exist(package_path):
        log("{0} already exists, skipping.".format(package_path))
        return

    # An empty level, not the default template: everything that belongs in the
    # range is spawned by the game mode when play starts.
    level_subsystem.new_level(package_path)

    set_game_mode_override(game_mode_path)

    level_subsystem.save_current_level()
    log("Created {0}.".format(package_path))


def main():
    for package_path, game_mode_path in MAPS:
        create_map(package_path, game_mode_path)

    log("Done. Set Edit > Project Settings > Maps & Modes if the defaults did not apply.")


main()
