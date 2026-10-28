$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$spec=Get-Content (Join-Path $PSScriptRoot 'ort-dependency.json') | ConvertFrom-Json
$folder=Join-Path $root "Dependencies/onnxruntime-win-x64-$($spec.version)"
if(Test-Path (Join-Path $folder 'include/onnxruntime_cxx_api.h')) { return }
New-Item -ItemType Directory -Force (Join-Path $root 'Dependencies') | Out-Null
$zip=Join-Path $root 'Dependencies/onnxruntime.zip'
Invoke-WebRequest -Uri $spec.url -OutFile $zip
if((Get-FileHash -Algorithm SHA256 $zip).Hash.ToLower() -ne $spec.sha256){ throw 'ONNX Runtime archive checksum mismatch' }
Expand-Archive -Force $zip (Join-Path $root 'Dependencies')
Remove-Item $zip
