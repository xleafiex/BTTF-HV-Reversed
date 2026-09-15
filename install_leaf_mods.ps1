param(
  [string]$Install = 'C:\Users\user\Desktop\Grand Theft Auto Vice City',
  [string]$Donor = 'E:\My Documents\crap\modding\BTTF HV\current wip\Grand Theft Auto Vice City (HV)'
)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $MyInvocation.MyCommand.Path
$mods=Join-Path $Install 'mods'
New-Item -ItemType Directory -Force $mods | Out-Null
& C:\Python314\python.exe (Join-Path $repo 'leaf-modules/package_delorean.py') --donor $Donor --output (Join-Path $mods 'delorean.leaf')
& C:\Python314\python.exe (Join-Path $repo 'leaf-modules/package_graphics.py') --output (Join-Path $mods 'graphics.leaf')
& C:\Python314\python.exe (Join-Path $repo 'leaf-modules/package_debug.py') --output (Join-Path $mods 'debug.leaf')
& C:\Python314\python.exe (Join-Path $repo 'leaf-modules/package_ragdoll.py') --output (Join-Path $mods 'ragdoll.leaf')
Copy-Item (Join-Path $repo 'leaf-modules/README.txt') (Join-Path $mods 'README.txt') -Force
Copy-Item (Join-Path $repo 'leaf-modules/DELOREAN-PORT-STATUS.md') (Join-Path $mods 'DELOREAN-PORT-STATUS.md') -Force
Copy-Item (Join-Path $repo 'build-mingw/src/reVC.exe') (Join-Path $Install 'reVC.exe') -Force
Write-Host "Installed native Leaf modules to $mods"
