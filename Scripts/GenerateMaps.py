# Copyright zutemiss & dshadykh. Educational project.
#
# Creates the two maps the project needs, points each at its game mode, and
# opens the main menu when it is done.
#
# A .umap is a binary asset and this repository holds source only, so the maps
# are not committed. They are also the only two files the project cannot express
# in code - everything inside them is built at runtime by AFRRangeBuilder, so
# both maps stay completely empty.
#
# The script is safe to run more than once: maps that already exist are left
# untouched, and only their game mode override is refreshed.
#
# Ways to run it:
#
#   From a running editor: Window > Output Log, switch the console mode to
#   Python, then
#       exec(open(r"<project>/Scripts/GenerateMaps.py").read())
#
#   Together with the editor, from a terminal:
#       UnrealEditor "<project>/FiringRange.uproject" -ExecutePythonScript="<project>/Scripts/GenerateMaps.py"
#
# Creating the maps by hand works just as well. See the README for the steps.

import unreal

MAPS = [
    ("/Game/Maps/MainMenu", "/Script/FiringRange.FRMenuGameMode"),
    ("/Game/Maps/FiringRange", "/Script/FiringRange.FRRangeGameMode"),
]

# Map opened once everything is in place, so pressing Play starts at the menu.
START_MAP = "/Game/Maps/MainMenu"


def log(message):
    unreal.log("[FiringRange] {0}".format(message))


def warn(message):
    unreal.log_warning("[FiringRange] {0}".format(message))


def asset_exists(package_path):
    """Checks for an asset through the subsystem that ships with every editor."""
    subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    return subsystem.does_asset_exist(package_path)


def get_world_settings():
    """Returns the World Settings actor of the level currently open in the editor."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if world is None:
        return None

    # The World Settings actor is always present in a level, so searching for
    # it by class is enough and does not depend on how the level was created.
    return unreal.GameplayStatics.get_actor_of_class(world, unreal.WorldSettings)


def set_game_mode_override(game_mode_path):
    """Points the World Settings of the current level at one of the game modes.

    Without this a map falls back to the global default from DefaultEngine.ini,
    which is the menu game mode, so opening the range map directly and pressing
    Play would show a menu instead of a range. The packaged game does not depend
    on it, because the menu also passes the game mode in the travel URL.
    """
    settings = get_world_settings()
    if settings is None:
        warn("World Settings not found, set the game mode override by hand.")
        return False

    game_mode_class = unreal.load_class(None, game_mode_path)
    if game_mode_class is None:
        warn("Could not load {0}. Is the C++ module compiled?".format(game_mode_path))
        return False

    settings.set_editor_property("default_game_mode", game_mode_class)
    log("Game mode override set to {0}.".format(game_mode_path))
    return True


def ensure_map(package_path, game_mode_path):
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

    if asset_exists(package_path):
        # The map is already there: open it only to refresh its game mode.
        if not level_subsystem.load_level(package_path):
            warn("Could not open {0}.".format(package_path))
            return False
        log("{0} already exists.".format(package_path))
    else:
        # An empty level, not a template: everything that belongs in the range
        # is spawned by the game mode when play starts.
        if not level_subsystem.new_level(package_path):
            warn("Could not create {0}.".format(package_path))
            return False
        log("Created {0}.".format(package_path))

    set_game_mode_override(game_mode_path)

    # Saved again because the override above changed the level after creation.
    if not level_subsystem.save_current_level():
        warn("Could not save {0}.".format(package_path))
        return False

    return True


def main():
    succeeded = True

    for package_path, game_mode_path in MAPS:
        try:
            succeeded = ensure_map(package_path, game_mode_path) and succeeded
        except Exception as error:
            warn("Failed on {0}: {1}".format(package_path, error))
            succeeded = False

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if asset_exists(START_MAP):
        level_subsystem.load_level(START_MAP)

    if succeeded:
        log("Done. Both maps are ready, press Play to start at the main menu.")
    else:
        warn("Finished with problems, see the messages above.")


main()
