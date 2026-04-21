param(
    [Parameter(Mandatory = $true)]
    [string]$RootPath,

    [Parameter(Mandatory = $false)]
    [string]$OutputFile = "AES67_solution_export.txt"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Test-IsTextFile {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.FileInfo]$File
    )

    $textExtensions = @(
        ".sln", ".vcxproj", ".vcxproj.filters", ".props", ".targets",
        ".cpp", ".c", ".cc", ".cxx",
        ".hpp", ".h", ".hh", ".hxx",
        ".cs", ".json", ".xml", ".yml", ".yaml",
        ".md", ".txt", ".cmake", ".sh", ".ps1",
        ".bat", ".cmd", ".gitignore", ".editorconfig"
    )

    return $textExtensions -contains $File.Extension.ToLowerInvariant() -or
           $File.Name -in @(".gitignore", "CMakeLists.txt")
}

function Get-RelativePathSafe {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $base = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $base.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $base += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = [System.Uri]$base
    $targetUri = [System.Uri]([System.IO.Path]::GetFullPath($TargetPath))
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)
    $relativePath = [System.Uri]::UnescapeDataString($relativeUri.ToString())
    return $relativePath -replace '/', '\'
}

function Should-ExcludePath {
    param(
        [string]$RelativePath
    )

    $excludedFragments = @(
        "\.git\",
        "\.vs\",
        "\bin\",
        "\obj\",
        "\build\",
        "\x64\",
        "\Debug\",
        "\Release\",
        "\packages\",
        "\out\",
        "\CMakeFiles\"
    )

    foreach ($fragment in $excludedFragments) {
        if ($RelativePath -like "*$fragment*") {
            return $true
        }
    }

    return $false
}

$root = [System.IO.Path]::GetFullPath($RootPath)
if (-not (Test-Path -LiteralPath $root)) {
    throw "RootPath no existe: $root"
}

$outputPath = if ([System.IO.Path]::IsPathRooted($OutputFile)) {
    $OutputFile
} else {
    Join-Path $root $OutputFile
}

$allFiles = Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName

$textFiles = foreach ($file in $allFiles) {
    $relativePath = Get-RelativePathSafe -BasePath $root -TargetPath $file.FullName

    if (Should-ExcludePath -RelativePath ("\{0}\" -f $relativePath)) {
        continue
    }

    if (-not (Test-IsTextFile -File $file)) {
        continue
    }

    [PSCustomObject]@{
        RelativePath = $relativePath
        FullName     = $file.FullName
    }
}

$treeFiles = $textFiles | Select-Object -ExpandProperty RelativePath

$sb = New-Object System.Text.StringBuilder

[void]$sb.AppendLine("AES67 SOLUTION EXPORT")
[void]$sb.AppendLine("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
[void]$sb.AppendLine("Root: $root")
[void]$sb.AppendLine("")

[void]$sb.AppendLine("============================================================")
[void]$sb.AppendLine("FILE LIST")
[void]$sb.AppendLine("============================================================")
foreach ($path in $treeFiles) {
    [void]$sb.AppendLine($path)
}
[void]$sb.AppendLine("")

foreach ($file in $textFiles) {
    [void]$sb.AppendLine("============================================================")
    [void]$sb.AppendLine("FILE: $($file.RelativePath)")
    [void]$sb.AppendLine("============================================================")
    [void]$sb.AppendLine("")

    try {
        $content = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8
    }
    catch {
        $content = Get-Content -LiteralPath $file.FullName -Raw
    }

    [void]$sb.AppendLine($content)
    [void]$sb.AppendLine("")
}

[System.IO.File]::WriteAllText($outputPath, $sb.ToString(), [System.Text.Encoding]::UTF8)

Write-Host "Export completado:"
Write-Host $outputPath
Write-Host ""
Write-Host "Archivos incluidos: $($textFiles.Count)"