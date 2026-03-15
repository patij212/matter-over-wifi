$PROJECT_ROOT = "C:\mow"

$env:IDF_PATH = "$PROJECT_ROOT\esp-idf"
$env:ESP_MATTER_PATH = "$PROJECT_ROOT\esp-matter-v2"
$env:Path += ";$env:ESP_MATTER_PATH\tools\bin"
$env:ZAP_INSTALL_PATH = "$env:ESP_MATTER_PATH\connectedhomeip\connectedhomeip\.environment\cipd\packages\zap"

. "$PROJECT_ROOT\esp-idf\export.ps1"
Set-Location "$PROJECT_ROOT\firmware"
idf.py build 2>&1 | Tee-Object -FilePath build_log.txt
