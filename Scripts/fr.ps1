# Firing Range helper for Windows.
#
# Usage:  fr <command> [options]      (fr.cmd in the project root forwards here)
#         make <command>              (if GNU make is installed)
#
# Commands:
#   help              show this list
#   check             check the computer, the tools and the project
#   setup             install what is missing, then build the project
#   build             compile the C++ module for the editor
#   maps [force]      create the two empty maps if they are missing
#   run [args]        start the game without the editor
#   editor            open the project in Unreal Editor
#   package [linux]   build a standalone Shipping game and zip it
#   clean [all]       delete build output
#   push              authors only: send main to Gitea and sync the GitHub mirror
#
# Unreal Engine is found automatically. Set UE_ROOT to use a specific copy.
#
# The file is plain ASCII on purpose: Windows PowerShell 5.1 reads scripts
# without a byte order mark in the system code page.

$ErrorActionPreference = 'Stop'

$ProjectRoot   = Split-Path -Parent $PSScriptRoot
$ProjectFile   = Join-Path $ProjectRoot 'FiringRange.uproject'
$EngineVersion = '5.5'

# Visual Studio 2022 17.8 or newer, with the two workloads Unreal needs.
$VsVersionRange = '[17.8,18.0)'
$VsWorkloads    = @('Microsoft.VisualStudio.Workload.NativeGame', 'Microsoft.VisualStudio.Workload.NativeDesktop')

$MinCores = 4
$MinRamGB = 8

# Free space: the engine takes about 45 GB, Visual Studio about 20 GB,
# the project build and a packaged game about 10 GB more.
$MinFreeGBWithEngine    = 15
$MinFreeGBWithoutEngine = 80

$EpicLauncherMsiUrl = 'https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/installer/download/EpicGamesLauncherInstaller.msi'
$VsBootstrapperUrl  = 'https://aka.ms/vs/17/release/vs_community.exe'

$ToolNames = @{ git = 'Git'; vs = 'Visual Studio 2022'; launcher = 'Epic Games Launcher' }
$MapNames  = @('MainMenu', 'FiringRange')

$script:Problems = 0

# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------

function Write-Status([string]$Level, [string]$Text) {
    $colors = @{ ok = 'Green'; warn = 'Yellow'; fail = 'Red'; info = 'Cyan' }
    Write-Host ('  [{0,-4}] ' -f $Level) -ForegroundColor $colors[$Level] -NoNewline
    Write-Host $Text
    if ($Level -eq 'fail') { $script:Problems++ }
}

function Write-Hint([string]$Text) {
    Write-Host "         $Text" -ForegroundColor DarkGray
}

function Write-Title([string]$Text) {
    Write-Host ''
    Write-Host $Text -ForegroundColor Cyan
}

function Stop-WithError([string]$Text) {
    Write-Host ''
    Write-Host "Error: $Text" -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# Finding things
# ---------------------------------------------------------------------------

function Test-Admin {
    $principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-Command([string]$Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Update-SessionPath {
    # Programs installed a moment ago are not on PATH of this window yet.
    $machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $user    = [Environment]::GetEnvironmentVariable('Path', 'User')
    $env:Path = "$machine;$user"
}

function Find-Engine {
    $candidates = New-Object System.Collections.Generic.List[string]
    if ($env:UE_ROOT) { $candidates.Add($env:UE_ROOT) }

    # Written by the engine installer. Not every install has it.
    $key = Get-ItemProperty -Path "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$EngineVersion" -ErrorAction SilentlyContinue
    if ($key -and $key.InstalledDirectory) { $candidates.Add($key.InstalledDirectory) }

    # The launcher's own list of what it installed.
    $manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path $manifest) {
        try {
            foreach ($entry in (Get-Content $manifest -Raw | ConvertFrom-Json).InstallationList) {
                if ($entry -and $entry.AppName -eq "UE_$EngineVersion") { $candidates.Add($entry.InstallLocation) }
            }
        } catch { }
    }

    # Default folders on every fixed drive.
    foreach ($drive in [IO.DriveInfo]::GetDrives()) {
        if (-not $drive.IsReady -or $drive.DriveType -ne 'Fixed') { continue }
        $root = $drive.RootDirectory.FullName
        $candidates.Add((Join-Path $root "Program Files\Epic Games\UE_$EngineVersion"))
        $candidates.Add((Join-Path $root "Epic Games\UE_$EngineVersion"))
    }

    foreach ($path in $candidates) {
        if ($path -and (Test-Path (Join-Path $path 'Engine\Build\BatchFiles\Build.bat'))) {
            return (Resolve-Path $path).Path
        }
    }
    return $null
}

function Get-EngineOrStop {
    $engine = Find-Engine
    if (-not $engine) {
        Stop-WithError "Unreal Engine $EngineVersion was not found. Run 'fr setup' for instructions, or set UE_ROOT to the engine folder."
    }
    return $engine
}

function Get-EngineVersionText([string]$Engine) {
    try {
        $v = Get-Content (Join-Path $Engine 'Engine\Build\Build.version') -Raw | ConvertFrom-Json
        return '{0}.{1}.{2}' -f $v.MajorVersion, $v.MinorVersion, $v.PatchVersion
    } catch {
        return 'unknown'
    }
}

function Get-EditorPath([string]$Engine) {
    return Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
}

function Get-VsWhere {
    $path = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $path) { return $path }
    return $null
}

function Find-VisualStudio {
    # A Visual Studio 2022 install that has every workload Unreal needs.
    $vswhere = Get-VsWhere
    if (-not $vswhere) { return $null }
    $query = @('-products', '*', '-version', $VsVersionRange, '-property', 'installationPath', '-requires') + $VsWorkloads
    return (& $vswhere @query | Select-Object -First 1)
}

function Find-VisualStudioIde {
    # Any Visual Studio 2022 IDE, even without the workloads.
    $vswhere = Get-VsWhere
    if (-not $vswhere) { return $null }
    $products = @('Microsoft.VisualStudio.Product.Community', 'Microsoft.VisualStudio.Product.Professional', 'Microsoft.VisualStudio.Product.Enterprise')
    $query = @('-version', $VsVersionRange, '-property', 'installationPath', '-products') + $products
    return (& $vswhere @query | Select-Object -First 1)
}

function Get-VisualStudioVersion([string]$Path) {
    $vswhere = Get-VsWhere
    return (& $vswhere -path $Path -property catalog_productDisplayVersion | Select-Object -First 1)
}

function Find-Launcher {
    $paths = @(
        (Join-Path $env:ProgramFiles 'Epic Games\Launcher\Portal\Binaries\Win64\EpicGamesLauncher.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Epic Games\Launcher\Portal\Binaries\Win64\EpicGamesLauncher.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Epic Games\Launcher\Portal\Binaries\Win32\EpicGamesLauncher.exe')
    )
    foreach ($path in $paths) {
        if (Test-Path $path) { return $path }
    }
    return $null
}

function Invoke-Silently([scriptblock]$Block) {
    # Windows PowerShell turns redirected stderr of a program into errors,
    # and 'Stop' would make them fatal. Only the exit code matters here.
    $ErrorActionPreference = 'Continue'
    & $Block *> $null
    return ($LASTEXITCODE -eq 0)
}

function Test-GitLfs {
    if (-not (Test-Command 'git')) { return $false }
    return (Invoke-Silently { git lfs version })
}

function Get-ModuleBinary {
    return Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-FiringRange.dll'
}

function Test-ModuleCurrent {
    # True when the editor module exists and nothing in Source is newer.
    $binary = Get-ModuleBinary
    if (-not (Test-Path $binary)) { return $false }
    $built = (Get-Item $binary).LastWriteTimeUtc
    if ((Get-Item $ProjectFile).LastWriteTimeUtc -gt $built) { return $false }
    $newer = Get-ChildItem (Join-Path $ProjectRoot 'Source') -Recurse -File |
        Where-Object { $_.LastWriteTimeUtc -gt $built } |
        Select-Object -First 1
    return (-not $newer)
}

function Get-MissingMaps {
    return @($MapNames | Where-Object { -not (Test-Path (Join-Path $ProjectRoot "Content\Maps\$_.umap")) })
}

# ---------------------------------------------------------------------------
# check
# ---------------------------------------------------------------------------

function Test-Environment {
    $state = @{}
    $engine = Find-Engine

    Write-Title 'Computer'

    $os = Get-CimInstance Win32_OperatingSystem
    if ($os.OSArchitecture -match '64') {
        Write-Status ok "$($os.Caption) $($os.Version), 64-bit"
    } else {
        Write-Status fail "$($os.Caption) is 32-bit. Unreal Engine needs 64-bit Windows."
    }

    $cs = Get-CimInstance Win32_ComputerSystem
    $cores = $cs.NumberOfLogicalProcessors
    if ($cores -ge $MinCores) {
        Write-Status ok "CPU: $cores logical cores"
    } else {
        Write-Status warn "CPU: $cores logical cores. Unreal asks for at least $MinCores; builds will be slow."
    }

    $ramGB = [math]::Round($cs.TotalPhysicalMemory / 1GB, 1)
    if ($ramGB -ge $MinRamGB) {
        Write-Status ok "RAM: $ramGB GB"
    } else {
        Write-Status warn "RAM: $ramGB GB. Unreal asks for at least $MinRamGB GB."
        Write-Hint 'It still works: the build runs fewer jobs at once and the editor is slower.'
        Write-Hint 'Close the browser and other programs before building.'
    }

    $gpus = @(Get-CimInstance Win32_VideoController | ForEach-Object { $_.Name }) -join ', '
    Write-Status info "GPU: $gpus"
    Write-Hint 'Needs DirectX 11 or 12 and a current driver.'

    $needGB = $MinFreeGBWithEngine
    if (-not $engine) { $needGB = $MinFreeGBWithoutEngine }
    $root = [IO.Path]::GetPathRoot($ProjectRoot).TrimEnd([char]92)
    $freeGB = [math]::Round((New-Object IO.DriveInfo($root)).AvailableFreeSpace / 1GB, 1)
    if ($freeGB -ge $needGB) {
        Write-Status ok "Free space on ${root} $freeGB GB"
    } else {
        Write-Status warn "Free space on ${root} $freeGB GB, about $needGB GB is needed."
        if (-not $engine) { Write-Hint 'Engine ~45 GB + Visual Studio ~20 GB + project build ~10 GB.' }
    }

    Write-Title 'Tools'

    $state.Git = Test-Command 'git'
    if ($state.Git) {
        Write-Status ok ((& git --version) -replace '^git version ', 'Git ')
        if (Test-GitLfs) {
            Write-Status ok ((& git lfs version) -replace '^git-lfs/(\S+).*', 'Git LFS $1')
        } else {
            Write-Status warn 'Git LFS not found. Only needed for the ready-made build archive on GitHub.'
            Write-Hint 'Download: https://git-lfs.com'
        }
    } else {
        Write-Status fail 'Git not found.'
        Write-Hint 'Download: https://git-scm.com/download/win'
    }

    $vs = Find-VisualStudio
    $state.VisualStudio = [bool]$vs
    if ($vs) {
        Write-Status ok "Visual Studio $(Get-VisualStudioVersion $vs) with C++ game development"
        Write-Hint $vs
    } elseif (Find-VisualStudioIde) {
        Write-Status fail 'Visual Studio 2022 is installed, but the C++ workloads Unreal needs are missing.'
        Write-Hint 'Needed: "Game development with C++" and "Desktop development with C++".'
    } else {
        Write-Status fail 'Visual Studio 2022 not found.'
        Write-Hint 'Download: https://visualstudio.microsoft.com/vs/community/'
    }

    $launcher = Find-Launcher
    $state.Launcher = [bool]$launcher
    if ($launcher) {
        Write-Status ok 'Epic Games Launcher'
    } elseif ($engine) {
        Write-Status info 'Epic Games Launcher not found. Not needed: the engine is already installed.'
    } else {
        Write-Status fail 'Epic Games Launcher not found.'
        Write-Hint 'Download: https://store.epicgames.com/download'
    }

    $state.Engine = $engine
    if ($engine) {
        $version = Get-EngineVersionText $engine
        if ($version.StartsWith("$EngineVersion.")) {
            Write-Status ok "Unreal Engine $version"
        } else {
            Write-Status warn "Unreal Engine $version. The project is made for $EngineVersion."
        }
        Write-Hint $engine
    } else {
        Write-Status fail "Unreal Engine $EngineVersion not found."
        Write-Hint 'Install it in Epic Games Launcher, or set UE_ROOT to the engine folder.'
    }

    if ($env:LINUX_MULTIARCH_ROOT -and (Test-Path $env:LINUX_MULTIARCH_ROOT)) {
        Write-Status ok "Linux cross-compile toolchain: $env:LINUX_MULTIARCH_ROOT"
    } else {
        Write-Status info "Linux cross-compile toolchain not installed. Only needed for 'fr package linux'."
    }

    Write-Title 'Project'

    if (Test-ModuleCurrent) {
        Write-Status ok 'C++ module is built and up to date'
    } elseif (Test-Path (Get-ModuleBinary)) {
        Write-Status info "C++ module is older than the sources. 'fr build' or 'fr run' rebuilds it."
    } else {
        Write-Status info "C++ module is not built yet. 'fr build' or 'fr run' builds it."
    }

    $missingMaps = Get-MissingMaps
    if ($missingMaps.Count -eq 0) {
        Write-Status ok ('Maps: ' + ($MapNames -join ', '))
    } else {
        Write-Status fail ('Missing maps: ' + ($missingMaps -join ', ') + ". Run 'fr maps'.")
    }

    foreach ($zip in @(Get-ChildItem (Join-Path $ProjectRoot 'Packaged') -Filter 'FiringRange-*.zip' -ErrorAction SilentlyContinue)) {
        Write-Status info ('Packaged build: Packaged\{0} ({1:N0} MB)' -f $zip.Name, ($zip.Length / 1MB))
    }

    return $state
}

function Invoke-Check {
    Write-Host 'Firing Range - environment check (Windows)' -ForegroundColor Cyan
    $null = Test-Environment
    Write-Host ''
    if ($script:Problems -eq 0) {
        Write-Host 'Everything needed is in place.' -ForegroundColor Green
        exit 0
    }
    Write-Host "Problems found: $($script:Problems). 'fr setup' fixes what can be fixed automatically." -ForegroundColor Yellow
    exit 1
}

# ---------------------------------------------------------------------------
# setup
# ---------------------------------------------------------------------------

function Save-Download([string]$Url, [string]$Path) {
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    $ProgressPreference = 'SilentlyContinue'
    Write-Host "Downloading $Url"
    Invoke-WebRequest -Uri $Url -OutFile $Path -UseBasicParsing
}

function Install-VisualStudio([bool]$HasWinget) {
    $addArgs = ($VsWorkloads | ForEach-Object { "--add $_" }) -join ' '

    $ide = Find-VisualStudioIde
    if ($ide) {
        # Already installed: add the missing workloads to the existing copy.
        Write-Host "Adding C++ workloads to $ide"
        $installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
        $argLine = 'modify --installPath "{0}" {1} --includeRecommended --passive --norestart' -f $ide, $addArgs
        Start-Process -FilePath $installer -ArgumentList $argLine -Wait
        return
    }

    $override = "--passive --wait --norestart $addArgs --includeRecommended"
    if ($HasWinget) {
        & winget install --id Microsoft.VisualStudio.2022.Community -e --override $override --accept-package-agreements --accept-source-agreements
    } else {
        $bootstrapper = Join-Path $env:TEMP 'vs_community.exe'
        Save-Download $VsBootstrapperUrl $bootstrapper
        Start-Process -FilePath $bootstrapper -ArgumentList $override -Wait
    }
}

function Install-Launcher([bool]$HasWinget) {
    if ($HasWinget) {
        & winget install --id EpicGames.EpicGamesLauncher -e --accept-package-agreements --accept-source-agreements
    } else {
        $msi = Join-Path $env:TEMP 'EpicGamesLauncherInstaller.msi'
        Save-Download $EpicLauncherMsiUrl $msi
        Start-Process -FilePath 'msiexec.exe' -ArgumentList ('/i "{0}" /passive' -f $msi) -Wait
    }
}

function Install-Git([bool]$HasWinget) {
    if ($HasWinget) {
        & winget install --id Git.Git -e --accept-package-agreements --accept-source-agreements
    } else {
        Write-Host 'winget is not available. Install Git by hand: https://git-scm.com/download/win' -ForegroundColor Yellow
    }
}

function Install-Tools([string[]]$Items) {
    # Runs with administrator rights. Every installer here writes to Program Files.
    $hasWinget = Test-Command 'winget'
    foreach ($item in $Items) {
        switch ($item) {
            'git'      { Write-Host "`n=== Git ===" -ForegroundColor Cyan; Install-Git $hasWinget }
            'vs'       { Write-Host "`n=== Visual Studio 2022 ===" -ForegroundColor Cyan; Install-VisualStudio $hasWinget }
            'launcher' { Write-Host "`n=== Epic Games Launcher ===" -ForegroundColor Cyan; Install-Launcher $hasWinget }
        }
    }
    if ($Items -contains 'wait') {
        Write-Host ''
        Read-Host 'Finished. Press Enter to close this window and continue'
    }
}

function Invoke-Elevated([string]$Command, [string[]]$Arguments) {
    $argLine = '-NoProfile -ExecutionPolicy Bypass -File "{0}" {1} {2}' -f $PSCommandPath, $Command, ($Arguments -join ' ')
    try {
        $process = Start-Process -FilePath 'powershell.exe' -ArgumentList $argLine -Verb RunAs -Wait -PassThru
        return $process.ExitCode
    } catch {
        # The user pressed "No" in the UAC prompt.
        return -1
    }
}

function Show-EngineInstructions {
    Write-Status warn "Unreal Engine $EngineVersion is not installed yet."
    Write-Host '         Epic gives the engine only through its launcher, so this step is yours:'
    Write-Host '           1. Open Epic Games Launcher and sign in. A free account: https://www.epicgames.com/id/register'
    Write-Host '           2. Unreal Engine tab -> Library -> the yellow "+" next to Engine Versions.'
    Write-Host "           3. On the new slot open the version list and pick $EngineVersion.x, not the newest one."
    Write-Host '           4. Install. In Options you may untick Starter Content, Templates and other platforms.'
    Write-Host "           5. When it finishes, run 'fr setup' again."
    Write-Hint 'A brand-new Epic account can see an empty version list for a few hours. Try again later.'

    $launcher = Find-Launcher
    if ($launcher) { Start-Process -FilePath $launcher }
}

function Invoke-Setup {
    Write-Host 'Firing Range - setup (Windows)' -ForegroundColor Cyan
    Write-Hint 'Finished steps are skipped, so it is safe to run this again.'

    $state = Test-Environment

    $missing = @()
    if (-not $state.Git)          { $missing += 'git' }
    if (-not $state.VisualStudio) { $missing += 'vs' }
    if (-not $state.Engine -and -not $state.Launcher) { $missing += 'launcher' }

    Write-Title 'Step 1 of 5. Tools'
    if ($missing.Count -eq 0) {
        Write-Status ok 'Nothing to install'
    } else {
        Write-Status info ('Installing: ' + (($missing | ForEach-Object { $ToolNames[$_] }) -join ', '))
        if ($missing -contains 'vs') {
            Write-Hint 'Visual Studio downloads about 12 GB and takes 30-60 minutes. Do not close its window.'
        }
        if (Test-Admin) {
            Install-Tools $missing
        } else {
            Write-Hint 'Windows will ask for administrator rights. Press "Yes".'
            if ((Invoke-Elevated 'install-tools' ($missing + 'wait')) -lt 0) {
                Stop-WithError "administrator rights were not granted. Run 'fr setup' again and press 'Yes'."
            }
        }
        Update-SessionPath

        if (-not (Find-VisualStudio)) {
            Stop-WithError ('Visual Studio still lacks the C++ workloads. Open "Visual Studio Installer", press Modify, ' +
                'tick "Game development with C++" and "Desktop development with C++", then run setup again.')
        }
        Write-Status ok 'Tools installed'
    }

    Write-Title 'Step 2 of 5. Git LFS'
    if (-not (Test-Path (Join-Path $ProjectRoot '.git'))) {
        Write-Status info 'Not a git clone, skipped'
    } elseif (Test-GitLfs) {
        $null = Invoke-Silently { git -C $ProjectRoot lfs install --local }
        Write-Status ok 'Git LFS hooks are installed for this repository'
    } else {
        Write-Status warn 'Git LFS not found, skipped. Only needed for the build archive on GitHub.'
    }

    Write-Title "Step 3 of 5. Unreal Engine $EngineVersion"
    $engine = Find-Engine
    if (-not $engine) {
        Show-EngineInstructions
        exit 2
    }
    Write-Status ok $engine

    Write-Title 'Step 4 of 5. Build'
    Build-Project $engine @()

    Write-Title 'Step 5 of 5. Maps'
    New-Maps $engine $false

    Write-Title 'Done'
    Write-Host '  Start the game:      fr run'
    Write-Host '  Open the editor:     fr editor'
    Write-Host '  Make a Shipping zip: fr package'
}

# ---------------------------------------------------------------------------
# build, maps, run, editor
# ---------------------------------------------------------------------------

function Build-Project([string]$Engine, [string[]]$Extra) {
    $buildBat = Join-Path $Engine 'Engine\Build\BatchFiles\Build.bat'
    Write-Hint 'The first build takes 5-15 minutes. Unreal limits parallel jobs to the free memory itself.'
    & $buildBat FiringRangeEditor Win64 Development "-Project=$ProjectFile" -WaitMutex -NoHotReload @Extra
    if ($LASTEXITCODE -ne 0) {
        Stop-WithError "the build failed (exit code $LASTEXITCODE). If the editor is open, close it and try again."
    }
    Write-Status ok 'C++ module is built'
}

function Build-IfNeeded([string]$Engine) {
    if (Test-ModuleCurrent) { return }
    Write-Title 'Building the C++ module first'
    Build-Project $Engine @()
}

function New-Maps([string]$Engine, [bool]$Force) {
    $missing = Get-MissingMaps
    if ($missing.Count -eq 0 -and -not $Force) {
        Write-Status ok "Both maps are already in Content\Maps. 'fr maps force' recreates them."
        return
    }

    Build-IfNeeded $Engine
    $pythonScript = Join-Path $PSScriptRoot 'GenerateMaps.py'
    $argLine = '"{0}" -ExecutePythonScript="{1}" -ExecCmds="scalability 0"' -f $ProjectFile, $pythonScript
    $started = Get-Date
    Start-Process -FilePath (Get-EditorPath $Engine) -ArgumentList $argLine
    Write-Status info 'The editor is starting and will create the maps. The first start can take 10-40 minutes.'

    # Wait until both files are written by this run.
    $deadline = $started.AddMinutes(60)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 10
        $written = @($MapNames | Where-Object {
            $file = Join-Path $ProjectRoot "Content\Maps\$_.umap"
            (Test-Path $file) -and ((Get-Item $file).LastWriteTime -ge $started)
        })
        if ($written.Count -eq $MapNames.Count) {
            Write-Status ok 'Maps saved. The editor stays open; close it when you are done.'
            return
        }
    }
    Stop-WithError 'the maps did not appear within an hour. Check the Output Log in the editor.'
}

function Start-Game([string]$Engine, [string[]]$Extra) {
    Build-IfNeeded $Engine
    # Options after 'run' replace the default window settings.
    $options = '-windowed -ResX=1280 -ResY=720'
    if ($Extra.Count -gt 0) {
        # PowerShell drops the quotes, so put them back around values with spaces.
        $options = ($Extra | ForEach-Object {
            if ($_ -match '^(-[^=\s]+=)(.*\s.*)$') { '{0}"{1}"' -f $Matches[1], $Matches[2] } else { $_ }
        }) -join ' '
    }
    $argLine = '"{0}" -game {1}' -f $ProjectFile, $options
    Write-Status info "Starting the game: UnrealEditor.exe $argLine"
    Start-Process -FilePath (Get-EditorPath $Engine) -ArgumentList $argLine
}

function Start-Editor([string]$Engine) {
    Build-IfNeeded $Engine
    Write-Status info 'Opening the editor. The first start compiles shaders for 10-40 minutes.'
    Start-Process -FilePath (Get-EditorPath $Engine) -ArgumentList ('"{0}"' -f $ProjectFile)
}

# ---------------------------------------------------------------------------
# package
# ---------------------------------------------------------------------------

function New-Package([string]$Engine, [string]$Target) {
    switch ($Target) {
        ''      { $platform = 'Win64'; $stageName = 'Windows' }
        'win64' { $platform = 'Win64'; $stageName = 'Windows' }
        'linux' { $platform = 'Linux'; $stageName = 'Linux' }
        default { Stop-WithError "unknown platform '$Target'. Use 'fr package' or 'fr package linux'." }
    }

    if ($platform -eq 'Linux' -and -not ($env:LINUX_MULTIARCH_ROOT -and (Test-Path $env:LINUX_MULTIARCH_ROOT))) {
        $sdk = Get-Content (Join-Path $Engine 'Engine\Config\Linux\Linux_SDK.json') -Raw
        $toolchain = [regex]::Match($sdk, '"MainVersion"\s*:\s*"([^"]+)"').Groups[1].Value
        Write-Status fail 'The Linux cross-compile toolchain is not installed.'
        Write-Hint "Download and install: https://cdn.unrealengine.com/CrossToolchain_Linux/$toolchain.exe"
        Write-Hint 'The installer sets LINUX_MULTIARCH_ROOT. Open a new terminal afterwards.'
        exit 1
    }

    Build-IfNeeded $Engine

    $archiveDir = Join-Path $ProjectRoot 'Packaged'
    $uatArgs = @(
        'BuildCookRun', "-project=$ProjectFile", '-noP4',
        "-platform=$platform", '-clientconfig=Shipping',
        '-build', '-cook', '-stage', '-pak', '-compressed',
        '-archive', "-archivedirectory=$archiveDir",
        '-nocompileeditor', '-unattended', '-utf8output'
    )
    if ($platform -eq 'Win64') { $uatArgs += '-prereqs' }

    Write-Title "Packaging a Shipping build for $platform"
    Write-Hint 'The first cook compiles about 1300 shaders and can take an hour on a weak laptop.'
    & (Join-Path $Engine 'Engine\Build\BatchFiles\RunUAT.bat') @uatArgs
    if ($LASTEXITCODE -ne 0) { Stop-WithError "packaging failed (exit code $LASTEXITCODE)." }

    $stage = Join-Path $archiveDir $stageName
    $zip = Join-Path $archiveDir "FiringRange-$platform.zip"
    if (Test-Path $zip) { Remove-Item $zip -Force }

    # Windows' own tar writes standard zip files with forward slashes.
    # Debug symbols and manifests are left out: the game does not need them.
    $tar = Join-Path $env:SystemRoot 'System32\tar.exe'
    if (-not (Test-Path $tar)) { Stop-WithError "tar.exe not found. The build is ready in $stage; zip it by hand." }
    $items = @(Get-ChildItem $stage | ForEach-Object { $_.Name })
    & $tar -a -c -f $zip --exclude '*.pdb' --exclude '*.debug' --exclude '*.sym' --exclude 'Manifest_*' -C $stage @items
    if ($LASTEXITCODE -ne 0) { Stop-WithError 'could not create the zip file.' }

    Write-Status ok ('{0} ({1:N0} MB)' -f $zip, ((Get-Item $zip).Length / 1MB))
    if ($platform -eq 'Linux') {
        Write-Hint 'Zip files made on Windows lose the Unix "executable" flag. On Linux run:'
        Write-Hint '  chmod +x FiringRange.sh FiringRange/Binaries/Linux/*'
    }
}

# ---------------------------------------------------------------------------
# clean, push
# ---------------------------------------------------------------------------

function Invoke-Clean([string]$Mode) {
    $folders = @('Binaries', 'Intermediate')
    if ($Mode -eq 'all') { $folders += @('DerivedDataCache', 'Packaged') }
    foreach ($name in $folders) {
        $path = Join-Path $ProjectRoot $name
        if (Test-Path $path) {
            Remove-Item -Path $path -Recurse -Force
            Write-Status ok "Removed $name"
        }
    }
    if ($Mode -ne 'all') { Write-Hint "'fr clean all' also removes DerivedDataCache and Packaged." }
}

function Invoke-Git([string[]]$GitArgs) {
    & git -C $ProjectRoot @GitArgs
    if ($LASTEXITCODE -ne 0) { Stop-WithError "git $($GitArgs -join ' ') failed." }
}

function Invoke-Push {
    # main goes to the school Gitea. The GitHub mirror gets the same history
    # plus the build archive, kept on the github-release branch.
    if (-not (Test-Command 'git')) { Stop-WithError 'Git is not installed.' }

    $branch = (& git -C $ProjectRoot rev-parse --abbrev-ref HEAD).Trim()
    if ($branch -ne 'main') { Stop-WithError "switch to main first (current branch: $branch)." }
    if (& git -C $ProjectRoot status --porcelain --untracked-files=no) { Stop-WithError 'commit or stash your changes first.' }

    Write-Title 'Gitea: main'
    Invoke-Git @('push', 'origin', 'main')

    $remotes = @(& git -C $ProjectRoot remote)
    $hasRelease = Invoke-Silently { git -C $ProjectRoot show-ref --verify --quiet refs/heads/github-release }
    if (($remotes -notcontains 'github') -or -not $hasRelease) {
        Write-Status info "No 'github' remote or 'github-release' branch here. The mirror step is skipped."
        return
    }
    if (-not (Test-GitLfs)) { Stop-WithError 'Git LFS is required to update the GitHub mirror.' }

    Write-Title 'GitHub: github-release -> main'
    Invoke-Git @('checkout', '-q', 'github-release')
    & git -C $ProjectRoot merge -q --no-edit main
    if ($LASTEXITCODE -ne 0) {
        & git -C $ProjectRoot merge --abort
        & git -C $ProjectRoot checkout -q main
        Stop-WithError 'merging main into github-release hit a conflict. Resolve it by hand.'
    }
    & git -C $ProjectRoot push github github-release:main
    $pushCode = $LASTEXITCODE
    Invoke-Git @('checkout', '-q', 'main')
    if ($pushCode -ne 0) { Stop-WithError 'pushing to GitHub failed.' }
    Write-Status ok 'Both repositories are up to date'
}

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

function Show-Help {
    Write-Host @'
Firing Range helper (Windows)

  fr check            check the computer, the tools and the project
  fr setup            install what is missing, then build the project
  fr build            compile the C++ module for the editor
  fr maps [force]     create the two empty maps if they are missing
  fr run [options]    start the game without the editor
                      options replace the window settings, e.g. fr run -fullscreen
  fr editor           open the project in Unreal Editor
  fr package [linux]  build a Shipping game into Packaged\ and zip it
  fr clean [all]      delete Binaries and Intermediate (all: also DerivedDataCache, Packaged)
  fr push             authors only: update Gitea and the GitHub mirror

In PowerShell type .\fr instead of fr. With GNU make installed, 'make <command>' works too.
Set UE_ROOT to point at a specific Unreal Engine folder.
'@
}

$Command = 'help'
$Rest = @()
if ($args.Count -gt 0) { $Command = ([string]$args[0]).ToLowerInvariant() }
if ($args.Count -gt 1) { $Rest = @($args[1..($args.Count - 1)] | ForEach-Object { [string]$_ }) }
$Option = ''
if ($Rest.Count -gt 0) { $Option = $Rest[0].ToLowerInvariant() }

switch ($Command) {
    'help'          { Show-Help }
    'check'         { Invoke-Check }
    'setup'         { Invoke-Setup }
    'build'         { Build-Project (Get-EngineOrStop) $Rest }
    'maps'          { New-Maps (Get-EngineOrStop) ($Option -eq 'force') }
    'run'           { Start-Game (Get-EngineOrStop) $Rest }
    'editor'        { Start-Editor (Get-EngineOrStop) }
    'package'       { New-Package (Get-EngineOrStop) $Option }
    'clean'         { Invoke-Clean $Option }
    'push'          { Invoke-Push }
    'install-tools' { Install-Tools $Rest }
    default {
        Write-Host "Unknown command: $Command" -ForegroundColor Red
        Show-Help
        exit 1
    }
}
exit 0
