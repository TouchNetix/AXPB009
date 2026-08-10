param(
    [string[]]$Targets = @('STM32F070CB', 'STM32F072CB'),
    [switch]$SkipImageBuild
)

$ErrorActionPreference = 'Stop'
$imageName = 'axpb009-firmware'
$root = Split-Path -Parent $PSScriptRoot

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw 'Docker was not found on PATH. Install and start Docker Desktop first.'
}

Push-Location $root
try {
    if (-not $SkipImageBuild) {
        & docker build -t $imageName .
        if ($LASTEXITCODE -ne 0) {
            throw "Docker image build failed with exit code $LASTEXITCODE."
        }
    }

    $dockerArguments = @(
        'run'
        '--rm'
        '-v'
        "${root}:/workspace"
        $imageName
    ) + $Targets

    & docker @dockerArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Firmware build failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}
