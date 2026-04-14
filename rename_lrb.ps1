# 1. Configuration
$targetDir = "C:\Users\eljav\Documents\SOAR\LRBTemplateRepository"
$oldStr = "LRB"
$newStr = "LRB"

Write-Host "Starting Divide and Conquer: Replacing '$oldStr' with '$newStr'" -ForegroundColor Cyan

# 2. Step One: Content Replacement (Inside Files)
Write-Host "`n[Pass 1/2] Updating file contents..." -ForegroundColor Yellow
$files = Get-ChildItem -Path $targetDir -Recurse -File | Where-Object { $_.FullName -notmatch '\\\.git\\' }

foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw
    # Case-insensitive check and replace
    if ($content -match $oldStr) {
        $newContent = $content -replace $oldStr, $newStr
        $newContent | Set-Content $file.FullName -Encoding UTF8
        Write-Host "  Modified content in: $($file.Name)"
    }
}

# 3. Step Two: Renaming (Files and Folders)
Write-Host "`n[Pass 2/2] Renaming files and directories (Bottom-Up)..." -ForegroundColor Yellow
$items = Get-ChildItem -Path $targetDir -Recurse | 
         Where-Object { $_.FullName -notmatch '\\\.git\\' -and $_.Name -like "*$oldStr*" } | 
         Sort-Object { $_.FullName.Length } -Descending

foreach ($item in $items) {
    $newName = $item.Name -replace $oldStr, $newStr
    Rename-Item -Path $item.FullName -NewName $newName -ErrorAction SilentlyContinue
    Write-Host "  Renamed: $($item.Name) -> $newName"
}

Write-Host "`nMigration Complete!" -ForegroundColor Green
