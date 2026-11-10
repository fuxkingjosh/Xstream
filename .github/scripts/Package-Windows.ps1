[CmdletBinding()]
param(
    [ValidateSet('x64', 'arm64')]
    [string] $Target = 'x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'

if ( $DebugPreference -eq 'Continue' ) {
    $VerbosePreference = 'Continue'
    $InformationPreference = 'Continue'
}

if ( $env:CI -eq $null ) {
    throw "Package-Windows.ps1 requires CI environment"
}

if ( ! ( [System.Environment]::Is64BitOperatingSystem ) ) {
    throw "obs-studio requires a 64-bit system to build and run."
}

if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The obs-studio packaging script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Package {
    trap {
        Write-Error $_
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach( $Utility in $UtilityFunctions ) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    Install-BuildDependencies -WingetFile "${ScriptHome}/.Wingetfile"

    # Forks have no OBS version tags. git describe --tags --long then fails,
    # which used to abort packaging after CMake had already been fixed to
    # fall back to cmake/common/base-version.txt.
    $BaseVersion = (Get-Content -Path "$ProjectRoot/cmake/common/base-version.txt" -TotalCount 1).Trim()
    $GitDescription = $null
    try {
        $GitDescription = (& git describe --tags --long 2>$null | Select-Object -Last 1)
    } catch {
        $GitDescription = $null
    }
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($GitDescription) -or $GitDescription -notmatch '^\d+\.\d+\.\d+-') {
        $ShortSha = $null
        try {
            $ShortSha = (& git rev-parse --short=9 HEAD).Trim()
        } catch {
            $ShortSha = $null
        }
        if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($ShortSha)) {
            throw "Unable to determine a package version from git."
        }
        $GitDescription = "$BaseVersion-1-g$ShortSha"
    }
    $Tokens = ($GitDescription -split '-')
    $CommitVersion = $Tokens[0..$($Tokens.Count - 3)] -join '-'
    $CommitHash = $($Tokens[-1]).SubString(1)
    $CommitDistance = $Tokens[-2]

    if ( $CommitDistance -gt 0 ) {
        $OutputName = "obs-studio-${CommitVersion}-${CommitHash}"
    } else {
        $OutputName = "obs-studio-${CommitVersion}"
    }

    $CpackArgs = @(
        '-C', "${Configuration}"
    )

    if ( $DebugPreference -eq 'Continue' ) {
        $CpackArgs += ('--verbose')
    }

    Log-Group "Packaging obs-studio..."

    Push-Location -Stack PackageTemp "build_${Target}"

    cpack @CpackArgs

    $Package = Get-ChildItem -filter "obs-studio-*-windows-${Target}.zip" -File
    Move-Item -Path $Package -Destination "${OutputName}-windows-${Target}.zip"

    Pop-Location -Stack PackageTemp
}

Package
