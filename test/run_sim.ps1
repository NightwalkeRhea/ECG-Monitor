param(
    [ValidateSet('fir', 'spi-shifter', 'spi-rx', 'w25q64', 'all')]
    [string]$Test = 'spi-rx',
    [switch]$OpenWave
)

$ErrorActionPreference = 'Stop'

$toolRoots = @{
    Iverilog = 'C:\Users\rastg\Tools\iverilog\bin\iverilog.exe'
    Vvp      = 'C:\Users\rastg\Tools\iverilog\bin\vvp.exe'
    GtkWave  = 'C:\Users\rastg\Tools\iverilog\gtkwave\bin\gtkwave.exe'
}

function Resolve-Tool {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CommandName,
        [Parameter(Mandatory = $true)]
        [string]$FallbackPath
    )

    $cmd = Get-Command $CommandName -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    if (Test-Path $FallbackPath) {
        return $FallbackPath
    }

    throw "Required tool '$CommandName' was not found. Checked PATH and '$FallbackPath'."
}

$iverilog = Resolve-Tool -CommandName 'iverilog' -FallbackPath $toolRoots.Iverilog
$vvp = Resolve-Tool -CommandName 'vvp' -FallbackPath $toolRoots.Vvp
$gtkwave = Resolve-Tool -CommandName 'gtkwave' -FallbackPath $toolRoots.GtkWave

$repoRoot = Split-Path -Parent $PSScriptRoot
$fpgaRoot = Join-Path $repoRoot 'src\fpga'

$tests = @{
    'fir' = @{
        Testbench = Join-Path $PSScriptRoot 'sim\fir_filter_tb.v'
        Sources   = @(
            Join-Path $fpgaRoot 'fir_filter.v'
        )
        Output    = Join-Path $PSScriptRoot 'sim_fir.vvp'
        Waveform  = Join-Path $PSScriptRoot 'fir_filter_tb.vcd'
    }
    'spi-shifter' = @{
        Testbench = Join-Path $PSScriptRoot 'sim\spi_byte_shifter_tb.v'
        Sources   = @(
            Join-Path $fpgaRoot 'spi_shifter.v'
        )
        Output    = Join-Path $PSScriptRoot 'sim_spi_shifter.vvp'
        Waveform  = Join-Path $PSScriptRoot 'spi_byte_shifter_tb.vcd'
    }
    'spi-rx' = @{
        Testbench = Join-Path $PSScriptRoot 'sim\spi_slave_rx_tb.v'
        Sources   = @(
            Join-Path $fpgaRoot 'spi_slave_rx.v'
        )
        Output    = Join-Path $PSScriptRoot 'sim_spi_rx.vvp'
        Waveform  = Join-Path $PSScriptRoot 'spi_slave_rx_tb.vcd'
    }
    'w25q64' = @{
        Testbench = Join-Path $PSScriptRoot 'sim\w25q64_page_logger_tb.v'
        Sources   = @(
            Join-Path $fpgaRoot 'flash_page.v'
            Join-Path $fpgaRoot 'spi_shifter.v'
        )
        Output    = Join-Path $PSScriptRoot 'sim_w25q64.vvp'
        Waveform  = Join-Path $PSScriptRoot 'w25q64_page_logger_tb.vcd'
    }
}

function Invoke-Testbench {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $cfg = $tests[$Name]
    if (-not $cfg) {
        throw "Unknown testbench '$Name'."
    }

    Write-Host "Compiling $Name ..."
    $compileArgs = @(
        '-g2009'
        '-o'
        $cfg.Output
        $cfg.Testbench
    ) + $cfg.Sources
    & $iverilog @compileArgs | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed for '$Name'."
    }

    Write-Host "Running $Name ..."
    Push-Location $PSScriptRoot
    try {
        & $vvp $cfg.Output | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "Simulation failed for '$Name'."
        }
    } finally {
        Pop-Location
    }

    if (-not (Test-Path $cfg.Waveform)) {
        throw "Expected waveform '$($cfg.Waveform)' was not generated."
    }

    Write-Host "Waveform:" $cfg.Waveform
    return ,$cfg.Waveform
}

$selected = if ($Test -eq 'all') {
    @('fir', 'spi-shifter', 'spi-rx', 'w25q64')
} else {
    @($Test)
}

$waveforms = @()
foreach ($name in $selected) {
    $waveforms += Invoke-Testbench -Name $name
}

if ($OpenWave) {
    if ($waveforms.Count -ne 1) {
        throw "Use -OpenWave with a single testbench name, not 'all'."
    }

    Write-Host "Opening GTKWave ..."
    Start-Process -FilePath $gtkwave -ArgumentList $waveforms[0] | Out-Null
}
