#!/usr/bin/env bash
# Firing Range helper for macOS and Linux.
#
# Usage:  ./Scripts/fr.sh <command> [options]
#         make <command>
#
# Commands:
#   help              show this list
#   check             check the computer, the tools and the project
#   setup             install what is missing, then build the project
#   build             compile the C++ module for the editor
#   maps [force]      create the two empty maps if they are missing
#   run [args]        start the game without the editor
#   editor            open the project in Unreal Editor
#   package           build a standalone Shipping game and zip it
#   clean [all]       delete build output
#   push              authors only: send main to Gitea and sync the GitHub mirror
#
# Unreal Engine is found automatically. Set UE_ROOT to use a specific copy.
#
# Written for the bash 3.2 that macOS ships: no associative arrays,
# no ${var,,}, no mapfile.

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT_FILE="$PROJECT_ROOT/FiringRange.uproject"
ENGINE_VERSION="5.5"
MAP_NAMES="MainMenu FiringRange"

MIN_CORES=4
MIN_RAM_GB=8
# The engine takes about 40 GB, the project build and a packaged game about 10 GB,
# Xcode takes its own share on a Mac.
MIN_FREE_GB_WITH_ENGINE=15
MIN_FREE_GB_WITHOUT_ENGINE=65

XCODE_APP_STORE="macappstore://apps.apple.com/app/id497799835"
EPIC_LAUNCHER_DMG="https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/installer/download/EpicGamesLauncher.dmg"

case "$(uname -s)" in
  Darwin) HOST="Mac" ;;
  Linux)  HOST="Linux" ;;
  *)
    echo "Unsupported system: $(uname -s). On Windows use fr.cmd." >&2
    exit 1
    ;;
esac

PROBLEMS=0

# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------

if [ -t 1 ]; then
  RED=$'\033[31m'; GREEN=$'\033[32m'; YELLOW=$'\033[33m'; CYAN=$'\033[36m'; GREY=$'\033[90m'; RESET=$'\033[0m'
else
  RED=""; GREEN=""; YELLOW=""; CYAN=""; GREY=""; RESET=""
fi

status() {
  local color
  case "$1" in
    ok)   color="$GREEN" ;;
    warn) color="$YELLOW" ;;
    fail) color="$RED"; PROBLEMS=$((PROBLEMS + 1)) ;;
    *)    color="$CYAN" ;;
  esac
  printf '  %s[%-4s]%s %s\n' "$color" "$1" "$RESET" "$2"
}

hint()  { printf '         %s%s%s\n' "$GREY" "$1" "$RESET"; }
say()   { printf '         %s\n' "$1"; }
title() { printf '\n%s%s%s\n' "$CYAN" "$1" "$RESET"; }

die() {
  printf '\n%sError:%s %s\n' "$RED" "$RESET" "$1" >&2
  exit 1
}

# version_ge A B: true when dotted version A >= B.
version_ge() {
  awk -v a="$1" -v b="$2" 'BEGIN {
    na = split(a, x, "."); nb = split(b, y, ".")
    n = (na > nb) ? na : nb
    for (i = 1; i <= n; i++) {
      xi = (i <= na) ? x[i] + 0 : 0
      yi = (i <= nb) ? y[i] + 0 : 0
      if (xi > yi) exit 0
      if (xi < yi) exit 1
    }
    exit 0
  }'
}

# json_value FILE KEY: first "KEY": "value" in a simple JSON file.
json_value() {
  sed -n "s/^[[:space:]]*\"$2\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\".*/\1/p" "$1" | head -n 1
}

# ---------------------------------------------------------------------------
# Finding things
# ---------------------------------------------------------------------------

launcher_manifest_engine() {
  # The launcher's own list: InstallLocation comes before AppName in each entry.
  local manifest="$HOME/Library/Application Support/Epic/UnrealEngineLauncher/LauncherInstalled.dat"
  [ -f "$manifest" ] || return 0
  awk -v app="UE_$ENGINE_VERSION" -F'"' '
    /"InstallLocation"/ { location = $4 }
    /"AppName"/ && $4 == app { print location; exit }
  ' "$manifest"
}

find_engine() {
  local path
  for path in \
    "${UE_ROOT:-}" \
    "$(launcher_manifest_engine)" \
    "/Users/Shared/Epic Games/UE_$ENGINE_VERSION" \
    "$HOME/Epic Games/UE_$ENGINE_VERSION" \
    "$HOME/UnrealEngine" \
    "$HOME/UE_$ENGINE_VERSION" \
    "/opt/UnrealEngine" \
    "/opt/UE_$ENGINE_VERSION"
  do
    if [ -n "$path" ] && [ -f "$path/Engine/Build/BatchFiles/RunUAT.sh" ]; then
      (cd "$path" && pwd)
      return 0
    fi
  done
  return 1
}

engine_or_die() {
  ENGINE="$(find_engine)" || die "Unreal Engine $ENGINE_VERSION was not found. Run 'make setup' for instructions, or set UE_ROOT to the engine folder."
}

engine_version_text() {
  local file="$1/Engine/Build/Build.version"
  [ -f "$file" ] || { echo "unknown"; return; }
  awk -F'[:,]' '
    /"MajorVersion"/ { major = $2 + 0 }
    /"MinorVersion"/ { minor = $2 + 0 }
    /"PatchVersion"/ { patch = $2 + 0 }
    END { printf "%d.%d.%d\n", major, minor, patch }
  ' "$file"
}

editor_binary() {
  if [ "$HOST" = "Mac" ]; then
    echo "$1/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
  else
    echo "$1/Engine/Binaries/Linux/UnrealEditor"
  fi
}

module_binary() {
  if [ "$HOST" = "Mac" ]; then
    echo "$PROJECT_ROOT/Binaries/Mac/UnrealEditor-FiringRange.dylib"
  else
    echo "$PROJECT_ROOT/Binaries/Linux/libUnrealEditor-FiringRange.so"
  fi
}

module_current() {
  # True when the editor module exists and nothing in Source is newer.
  local binary
  binary="$(module_binary)"
  [ -f "$binary" ] || return 1
  [ "$PROJECT_FILE" -nt "$binary" ] && return 1
  [ -z "$(find "$PROJECT_ROOT/Source" -type f -newer "$binary" | head -n 1)" ]
}

missing_maps() {
  local name missing=""
  for name in $MAP_NAMES; do
    [ -f "$PROJECT_ROOT/Content/Maps/$name.umap" ] || missing="$missing $name"
  done
  echo "$missing" | sed 's/^ //'
}

has_git_lfs() {
  command -v git >/dev/null 2>&1 && git lfs version >/dev/null 2>&1
}

xcode_developer_dir() {
  # Honours DEVELOPER_DIR, so a second Xcode can be used without sudo.
  xcode-select -p 2>/dev/null
}

has_full_xcode() {
  local dir
  dir="$(xcode_developer_dir)"
  case "$dir" in
    ""|*CommandLineTools*) return 1 ;;
  esac
  [ -x "$dir/usr/bin/xcodebuild" ]
}

xcode_version() {
  xcodebuild -version 2>/dev/null | awk 'NR == 1 { print $2 }'
}

launcher_installed() {
  [ -d "/Applications/Epic Games Launcher.app" ] || [ -d "$HOME/Applications/Epic Games Launcher.app" ]
}

# ---------------------------------------------------------------------------
# check
# ---------------------------------------------------------------------------

check_xcode() {
  local engine="$1" version sdk min max others
  if ! has_full_xcode; then
    status fail "Xcode not found. Unreal needs the full Xcode, not only the Command Line Tools."
    hint "App Store: https://apps.apple.com/app/xcode/id497799835"
    others="$(ls -d /Applications/Xcode*.app 2>/dev/null | head -n 1)"
    if [ -n "$others" ]; then
      hint "Found $others. Select it without sudo:"
      hint "  export DEVELOPER_DIR=\"$others/Contents/Developer\""
    fi
    return
  fi

  version="$(xcode_version)"
  if [ -z "$version" ]; then
    status fail "Xcode is installed but does not start. Its license may not be accepted yet."
    hint "Open Xcode once, or run: sudo xcodebuild -license accept"
    return
  fi

  sdk="$engine/Engine/Config/Apple/Apple_SDK.json"
  if [ -n "$engine" ] && [ -f "$sdk" ]; then
    min="$(json_value "$sdk" MinVersion)"
    max="$(json_value "$sdk" MaxVersion)"
    if version_ge "$version" "$min" && version_ge "$max" "$version"; then
      status ok "Xcode $version (Unreal $ENGINE_VERSION accepts $min - $max)"
    else
      status fail "Xcode $version. Unreal $ENGINE_VERSION accepts only $min - $max."
      hint "Download a matching Xcode: https://developer.apple.com/download/all/"
      hint "Unpack it next to the current one and select it without sudo:"
      hint "  export DEVELOPER_DIR=/Applications/Xcode_<version>.app/Contents/Developer"
    fi
  else
    status ok "Xcode $version"
  fi
  hint "$(xcode_developer_dir)"
}

check_environment() {
  local engine cores ram_gb free_gb need_gb version missing zip

  engine="$(find_engine)"

  title "Computer"

  if [ "$HOST" = "Mac" ]; then
    status ok "macOS $(sw_vers -productVersion), $(uname -m)"
    cores="$(sysctl -n hw.ncpu)"
    ram_gb="$(sysctl -n hw.memsize | awk '{ printf "%.1f", $1 / 1073741824 }')"
    status info "CPU: $(sysctl -n machdep.cpu.brand_string 2>/dev/null)"
  else
    status ok "$( (. /etc/os-release 2>/dev/null && echo "$PRETTY_NAME") || uname -sr), $(uname -m)"
    cores="$(nproc 2>/dev/null || echo 1)"
    ram_gb="$(awk '/^MemTotal:/ { printf "%.1f", $2 / 1048576 }' /proc/meminfo)"
  fi

  if [ "$cores" -ge "$MIN_CORES" ]; then
    status ok "CPU cores: $cores"
  else
    status warn "CPU cores: $cores. Unreal asks for at least $MIN_CORES; builds will be slow."
  fi

  if version_ge "$ram_gb" "$MIN_RAM_GB"; then
    status ok "RAM: $ram_gb GB"
  else
    status warn "RAM: $ram_gb GB. Unreal asks for at least $MIN_RAM_GB GB."
    hint "It still works: the build runs fewer jobs at once and the editor is slower."
  fi

  need_gb="$MIN_FREE_GB_WITH_ENGINE"
  [ -z "$engine" ] && need_gb="$MIN_FREE_GB_WITHOUT_ENGINE"
  free_gb="$(df -Pk "$PROJECT_ROOT" | awk 'NR == 2 { printf "%d", $4 / 1048576 }')"
  if [ "$free_gb" -ge "$need_gb" ]; then
    status ok "Free space: $free_gb GB"
  else
    status warn "Free space: $free_gb GB, about $need_gb GB is needed."
  fi

  title "Tools"

  if command -v git >/dev/null 2>&1; then
    status ok "$(git --version | sed 's/^git version/Git/')"
    if has_git_lfs; then
      status ok "Git LFS $(git lfs version | sed 's|^git-lfs/\([^ ]*\).*|\1|')"
    else
      status warn "Git LFS not found. Only needed to push a build archive."
      if [ "$HOST" = "Mac" ]; then hint "brew install git-lfs   or   https://git-lfs.com"; else hint "https://git-lfs.com"; fi
    fi
  else
    status fail "Git not found."
    if [ "$HOST" = "Mac" ]; then hint "Run: xcode-select --install"; else hint "Install git with your package manager."; fi
  fi

  if [ "$HOST" = "Mac" ]; then
    check_xcode "$engine"
    if launcher_installed; then
      status ok "Epic Games Launcher"
    elif [ -n "$engine" ]; then
      status info "Epic Games Launcher not found. Not needed: the engine is already installed."
    else
      status fail "Epic Games Launcher not found."
      hint "Download: $EPIC_LAUNCHER_DMG"
    fi
  fi

  if [ -n "$engine" ]; then
    version="$(engine_version_text "$engine")"
    case "$version" in
      "$ENGINE_VERSION".*) status ok "Unreal Engine $version" ;;
      *) status warn "Unreal Engine $version. The project is made for $ENGINE_VERSION." ;;
    esac
    hint "$engine"
  else
    status fail "Unreal Engine $ENGINE_VERSION not found."
    if [ "$HOST" = "Mac" ]; then
      hint "Install it in Epic Games Launcher, or set UE_ROOT to the engine folder."
    else
      hint "Download the Linux build: https://www.unrealengine.com/linux, then set UE_ROOT."
    fi
  fi

  title "Project"

  if module_current; then
    status ok "C++ module is built and up to date"
  elif [ -f "$(module_binary)" ]; then
    status info "C++ module is older than the sources. 'make build' or 'make run' rebuilds it."
  else
    status info "C++ module is not built yet. 'make build' or 'make run' builds it."
  fi

  missing="$(missing_maps)"
  if [ -z "$missing" ]; then
    status ok "Maps: $MAP_NAMES"
  else
    status fail "Missing maps: $missing. Run 'make maps'."
  fi

  for zip in "$PROJECT_ROOT"/Packaged/FiringRange-*.zip; do
    [ -f "$zip" ] && status info "Packaged build: Packaged/$(basename "$zip") ($(du -m "$zip" | cut -f1) MB)"
  done
}

cmd_check() {
  printf '%sFiring Range - environment check (%s)%s\n' "$CYAN" "$HOST" "$RESET"
  check_environment
  echo
  if [ "$PROBLEMS" -eq 0 ]; then
    printf '%sEverything needed is in place.%s\n' "$GREEN" "$RESET"
    exit 0
  fi
  printf "%sProblems found: %d. 'make setup' explains how to fix them.%s\n" "$YELLOW" "$PROBLEMS" "$RESET"
  exit 1
}

# ---------------------------------------------------------------------------
# setup
# ---------------------------------------------------------------------------

show_engine_instructions() {
  status warn "Unreal Engine $ENGINE_VERSION is not installed yet."
  if [ "$HOST" = "Mac" ]; then
    say "Epic gives the engine only through its launcher, so this step is yours:"
    say "  1. Open Epic Games Launcher and sign in. A free account: https://www.epicgames.com/id/register"
    say "  2. Unreal Engine tab -> Library -> the yellow \"+\" next to Engine Versions."
    say "  3. On the new slot open the version list and pick $ENGINE_VERSION.x, not the newest one."
    say "  4. Install. In Options you may untick Starter Content, Templates and other platforms."
    say "  5. When it finishes, run 'make setup' again."
    hint "A brand-new Epic account can see an empty version list for a few hours. Try again later."
    launcher_installed && open -a "Epic Games Launcher" 2>/dev/null
  else
    say "Epic has no launcher for Linux:"
    say "  1. Sign in and download Unreal Engine $ENGINE_VERSION for Linux: https://www.unrealengine.com/linux"
    say "  2. Unpack it, for example to ~/UnrealEngine."
    say "  3. If it is somewhere else: export UE_ROOT=/path/to/engine"
    say "  4. Run 'make setup' again."
  fi
}

cmd_setup() {
  local engine
  printf '%sFiring Range - setup (%s)%s\n' "$CYAN" "$HOST" "$RESET"
  hint "Finished steps are skipped, so it is safe to run this again."

  check_environment
  engine="$(find_engine)"

  title "Step 1 of 5. Tools"
  command -v git >/dev/null 2>&1 || die "Git is required. On macOS run 'xcode-select --install'."

  if [ "$HOST" = "Mac" ]; then
    if ! has_full_xcode; then
      status warn "Install Xcode from the App Store, open it once, then run 'make setup' again."
      hint "It is a large download and needs your Apple ID, so the script cannot do it for you."
      open "$XCODE_APP_STORE" 2>/dev/null
      exit 2
    fi
    if [ -z "$engine" ] && ! launcher_installed; then
      if command -v brew >/dev/null 2>&1; then
        status info "Installing Epic Games Launcher with Homebrew"
        brew install --cask epic-games || die "Homebrew could not install the launcher. Download it by hand: $EPIC_LAUNCHER_DMG"
      else
        status warn "Download and install Epic Games Launcher, then run 'make setup' again:"
        hint "$EPIC_LAUNCHER_DMG"
        open "$EPIC_LAUNCHER_DMG" 2>/dev/null
        exit 2
      fi
    fi
  fi
  status ok "Tools are in place"

  title "Step 2 of 5. Git LFS"
  if [ ! -d "$PROJECT_ROOT/.git" ]; then
    status info "Not a git clone, skipped"
  elif has_git_lfs; then
    git -C "$PROJECT_ROOT" lfs install --local >/dev/null 2>&1
    status ok "Git LFS hooks are installed for this repository"
  else
    status warn "Git LFS not found, skipped. Only needed to push a build archive."
  fi

  title "Step 3 of 5. Unreal Engine $ENGINE_VERSION"
  if [ -z "$engine" ]; then
    show_engine_instructions
    exit 2
  fi
  status ok "$engine"
  ENGINE="$engine"

  title "Step 4 of 5. Build"
  build_project

  title "Step 5 of 5. Maps"
  make_maps ""

  title "Done"
  say "Start the game:      make run"
  say "Open the editor:     make editor"
  say "Make a Shipping zip: make package"
}

# ---------------------------------------------------------------------------
# build, maps, run, editor
# ---------------------------------------------------------------------------

build_project() {
  local script
  if [ "$HOST" = "Mac" ]; then
    script="$ENGINE/Engine/Build/BatchFiles/Mac/Build.sh"
  else
    script="$ENGINE/Engine/Build/BatchFiles/Linux/Build.sh"
  fi
  hint "The first build takes 5-15 minutes. Unreal limits parallel jobs to the free memory itself."
  bash "$script" FiringRangeEditor "$HOST" Development -Project="$PROJECT_FILE" -WaitMutex "$@" \
    || die "the build failed. If the editor is open, close it and try again."
  status ok "C++ module is built"
}

build_if_needed() {
  module_current && return 0
  title "Building the C++ module first"
  build_project
}

start_editor_process() {
  # Starts the editor in the background with the given arguments.
  if [ "$HOST" = "Mac" ]; then
    open -n -a "$ENGINE/Engine/Binaries/Mac/UnrealEditor.app" --args "$@"
  else
    nohup "$(editor_binary "$ENGINE")" "$@" >/dev/null 2>&1 &
  fi
}

make_maps() {
  local marker name waited
  if [ -z "$(missing_maps)" ] && [ "$1" != "force" ]; then
    status ok "Both maps are already in Content/Maps. 'make maps ARGS=force' recreates them."
    return 0
  fi

  build_if_needed
  marker="$(mktemp)"
  start_editor_process "$PROJECT_FILE" \
    -ExecutePythonScript="$PROJECT_ROOT/Scripts/GenerateMaps.py" \
    -ExecCmds="scalability 0"
  status info "The editor is starting and will create the maps. The first start can take 10-40 minutes."

  # Wait until both files are written by this run.
  waited=0
  while [ "$waited" -lt 3600 ]; do
    sleep 10
    waited=$((waited + 10))
    local done_count=0
    for name in $MAP_NAMES; do
      [ "$PROJECT_ROOT/Content/Maps/$name.umap" -nt "$marker" ] && done_count=$((done_count + 1))
    done
    if [ "$done_count" -eq 2 ]; then
      rm -f "$marker"
      status ok "Maps saved. The editor stays open; close it when you are done."
      return 0
    fi
  done
  rm -f "$marker"
  die "the maps did not appear within an hour. Check the Output Log in the editor."
}

cmd_run() {
  build_if_needed
  # Options after 'run' replace the default window settings.
  if [ "$#" -eq 0 ]; then
    set -- -windowed -ResX=1280 -ResY=720
  fi
  status info "Starting the game"
  "$(editor_binary "$ENGINE")" "$PROJECT_FILE" -game "$@"
}

cmd_editor() {
  build_if_needed
  status info "Opening the editor. The first start compiles shaders for 10-40 minutes."
  start_editor_process "$PROJECT_FILE"
}

# ---------------------------------------------------------------------------
# package
# ---------------------------------------------------------------------------

cmd_package() {
  local archive_dir="$PROJECT_ROOT/Packaged" stage zip app
  build_if_needed

  title "Packaging a Shipping build for $HOST"
  hint "The first cook compiles shaders and can take an hour on a weak computer."
  bash "$ENGINE/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
    -project="$PROJECT_FILE" -noP4 \
    -platform="$HOST" -clientconfig=Shipping \
    -build -cook -stage -pak -compressed \
    -archive -archivedirectory="$archive_dir" \
    -nocompileeditor -unattended -utf8output \
    || die "packaging failed."

  stage="$archive_dir/$HOST"
  zip="$archive_dir/FiringRange-$HOST.zip"
  rm -f "$zip"

  if [ "$HOST" = "Mac" ]; then
    app="$(ls -d "$stage"/*.app 2>/dev/null | head -n 1)"
    [ -n "$app" ] || die "no .app found in $stage."
    # ditto keeps the bundle structure, symlinks and permissions intact.
    ditto -c -k --sequesterRsrc --keepParent "$app" "$zip" || die "could not create the zip file."
    status ok "$zip ($(du -m "$zip" | cut -f1) MB)"
    hint "The app is not signed. On another Mac: right click -> Open, or run"
    hint "  xattr -cr FiringRange.app"
  else
    (cd "$stage" && zip -qr "$zip" . -x '*.debug' '*.sym') || die "could not create the zip file (is 'zip' installed?)."
    status ok "$zip ($(du -m "$zip" | cut -f1) MB)"
  fi
}

# ---------------------------------------------------------------------------
# clean, push
# ---------------------------------------------------------------------------

cmd_clean() {
  local folders="Binaries Intermediate" name
  [ "$1" = "all" ] && folders="$folders DerivedDataCache Packaged"
  for name in $folders; do
    if [ -d "$PROJECT_ROOT/$name" ]; then
      rm -rf "${PROJECT_ROOT:?}/$name"
      status ok "Removed $name"
    fi
  done
  [ "$1" = "all" ] || hint "'make clean ARGS=all' also removes DerivedDataCache and Packaged."
}

cmd_push() {
  # main goes to the school Gitea. The GitHub mirror gets the same history
  # plus the build archive, kept on the github-release branch.
  local branch push_code
  command -v git >/dev/null 2>&1 || die "Git is not installed."
  cd "$PROJECT_ROOT" || die "cannot enter $PROJECT_ROOT"

  branch="$(git rev-parse --abbrev-ref HEAD)"
  [ "$branch" = "main" ] || die "switch to main first (current branch: $branch)."
  [ -z "$(git status --porcelain --untracked-files=no)" ] || die "commit or stash your changes first."

  title "Gitea: main"
  git push origin main || die "git push origin main failed."

  if ! git remote | grep -qx github || ! git show-ref --verify --quiet refs/heads/github-release; then
    status info "No 'github' remote or 'github-release' branch here. The mirror step is skipped."
    return 0
  fi
  has_git_lfs || die "Git LFS is required to update the GitHub mirror."

  title "GitHub: github-release -> main"
  git checkout -q github-release || die "cannot switch to github-release."
  if ! git merge -q --no-edit main; then
    git merge --abort
    git checkout -q main
    die "merging main into github-release hit a conflict. Resolve it by hand."
  fi
  git push github github-release:main
  push_code=$?
  git checkout -q main || die "cannot switch back to main."
  [ "$push_code" -eq 0 ] || die "pushing to GitHub failed."
  status ok "Both repositories are up to date"
}

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

show_help() {
  cat <<EOF
Firing Range helper ($HOST)

  make check                  check the computer, the tools and the project
  make setup                  install what is missing, then build the project
  make build                  compile the C++ module for the editor
  make maps [ARGS=force]      create the two empty maps if they are missing
  make run [ARGS=...]         start the game without the editor
                              ARGS replace the window settings, e.g. ARGS=-fullscreen
  make editor                 open the project in Unreal Editor
  make package                build a Shipping game into Packaged/ and zip it
  make clean [ARGS=all]       delete Binaries and Intermediate (all: also DerivedDataCache, Packaged)
  make push                   authors only: update Gitea and the GitHub mirror

Without make: ./Scripts/fr.sh <command> [options]
Set UE_ROOT to point at a specific Unreal Engine folder.
EOF
}

COMMAND="${1:-help}"
[ "$#" -gt 0 ] && shift

case "$COMMAND" in
  help|-h|--help) show_help ;;
  check)   cmd_check ;;
  setup)   cmd_setup ;;
  build)   engine_or_die; build_project "$@" ;;
  maps)    engine_or_die; make_maps "${1:-}" ;;
  run)     engine_or_die; cmd_run "$@" ;;
  editor)  engine_or_die; cmd_editor ;;
  package) engine_or_die; cmd_package ;;
  clean)   cmd_clean "${1:-}" ;;
  push)    cmd_push ;;
  *)
    echo "Unknown command: $COMMAND" >&2
    show_help
    exit 1
    ;;
esac
