$projectPath = Convert-Path .
$tempPath = $env:temp + '\garage-door-client-build'

$repoUsername = "alexrcameron"
$repoName = "garage-door-client"
$repoVersionPattern = "v1.0.*"

$outputFilename = "ClientConfig.h"

If(Test-Path $tempPath) {
    Write-Host "Temp Dir Found!"
    cd $tempPath

    Write-Host -NoNewline "Fetching latest repository data... "
    git fetch
    Write-Host "Done!"
} Else {
    Write-Host -NoNewline "Creating Temp Directory... "
    New-Item -Path $tempPath -ItemType Directory | Out-Null
    Write-Host "Done!"

    Write-Host -NoNewline "Cloning repository... "
    git clone https://github.com/alexrcameron/garage-door-client --no-checkout $tempPath
    cd $tempPath
    git sparse-checkout init --cone
    git sparse-checkout set build
    Write-Host "Done!"
}

Write-Host -NoNewline "Finding release version... "
$repoVersion = @(git tag -l $repoVersionPattern --sort=-committerdate)[0]
Write-Host $repoVersion

Write-Host -NoNewline "Downloading index.html... "
$cdnPath = "http://cdn.jsdelivr.net/gh/$repoUsername/$repoName@$repoVersion/build/static"

#git ls-tree --name-only -r v1.0.0 build
$html = git show v1.0.0:build/index.html

$html = $html -replace "/static", $cdnPath
Write-Host "Done!"

Write-Host -NoNewline "Creating $outputFilename... "
$filePath = $projectPath + "\" + $outputFilename

$fileValue = "#ifndef CLIENT_CONFIG`n#define CLIENT_CONFIG`n`n"
$fileValue += "const char index_html[] PROGMEM = R""rawliteral($html)rawliteral"";"
$fileValue += "`n`n#endif"

Set-Content -Path $filePath -Value $fileValue
Write-Host "Done!"

Write-Host "press any key to exit..."
$Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
# SIG # Begin signature block
# MIIFiAYJKoZIhvcNAQcCoIIFeTCCBXUCAQExCzAJBgUrDgMCGgUAMGkGCisGAQQB
# gjcCAQSgWzBZMDQGCisGAQQBgjcCAR4wJgIDAQAABBAfzDtgWUsITrck0sYpfvNR
# AgEAAgEAAgEAAgEAAgEAMCEwCQYFKw4DAhoFAAQUi00HwaccS7/7nWzeBoNALj9l
# HTGgggMaMIIDFjCCAf6gAwIBAgIQQxNDmrsBZIVAo41t8OBxiTANBgkqhkiG9w0B
# AQUFADAjMSEwHwYDVQQDDBhBbGV4IENhbWVyb24gRGV2IFJvb3QgQ0EwHhcNMjAx
# MDI1MDUxMDIxWhcNMjExMDI1MDUzMDIxWjAjMSEwHwYDVQQDDBhBbGV4IENhbWVy
# b24gRGV2IFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDm
# cbCCR3XxxpnxX97Q+oyLn73PpjJXc11Kr4h1Nz/mYk71IJ/Ja5duhHktRTaff7VJ
# b2JOA2qf0xUe95C/5Q18JcbPtguNDJC92VV4EfRcfb9Ky0l5UKUtsiBaDVJwplCP
# 4PIu033vEFcIAK61UXLXKG+VXEvBJaPhXXtm7IxCQol8SiookNhaC8AYL5cNyvfy
# tHPJJoGropiKTuSWHZKZezGnvoYNPuq+fsXAK93MXg2DcXox41dFInN026265BUM
# BABuetAOppajHN6y9V4kyCqYuNab4G2LJLZoh1wD0pd6qdNN/0udmtFrDnlokYdZ
# WNKW1SmbeZg67BZ6ipDJAgMBAAGjRjBEMA4GA1UdDwEB/wQEAwIHgDATBgNVHSUE
# DDAKBggrBgEFBQcDAzAdBgNVHQ4EFgQUkwwuM+1x5b8swtPVwIa5ofMxGKAwDQYJ
# KoZIhvcNAQEFBQADggEBADQHoEORs/fjPt8tV6JYXVbj7fjXq0dFjVBfqR/DRGmI
# vtoCPWoD75i6L+HsOANHjuFROLQITEvOln7Tv0akkrvvGypXNPaOzpuA49YQ6NWg
# +wNco4rqybcofAGjogvNrssUEB8xDJT9Vbe+rfN69XBKYPvKAgcFeMe4q9irVQhZ
# 5mgR95F4Z759V0Fq8LeQsOZaryB7ukO9bjUxsFF6KqE83bPiaRijpXX1Qzqgh6PI
# lwTOZn2YNUDQh5L9J6k3x3YHKiPrySu4YjtUpLY7kk6KumZKIXlXW5p0f1SGdZa9
# al+bEtB4j/6jDfzU4eRKYwP2jIqkrg53glRCqsXaRxMxggHYMIIB1AIBATA3MCMx
# ITAfBgNVBAMMGEFsZXggQ2FtZXJvbiBEZXYgUm9vdCBDQQIQQxNDmrsBZIVAo41t
# 8OBxiTAJBgUrDgMCGgUAoHgwGAYKKwYBBAGCNwIBDDEKMAigAoAAoQKAADAZBgkq
# hkiG9w0BCQMxDAYKKwYBBAGCNwIBBDAcBgorBgEEAYI3AgELMQ4wDAYKKwYBBAGC
# NwIBFTAjBgkqhkiG9w0BCQQxFgQU9IHKdA0MQs2mWW8t1cS9GaG5sDQwDQYJKoZI
# hvcNAQEBBQAEggEAlxaTekI1vBj6PrrXI02O32aftTaaMx8Ad65xbJJYDccE1ku6
# HeZb0WvYTXE7Vh7DukNa2+ySb656drEUzRozLPqZvdRu3EM0f910uG1RAsy91DTw
# 0dGDfNykormVA4MPPxURwHSxmc6koggzqsYGXo/YBACiZHjXC/8yJ0j89YRyw2k0
# FARaDXdGPvXO5OaB+nrkjOi+aYj83igbkCoRV6FTTokx/4ixYHVmcbRhSHF3pUYe
# GsuLhsf9D78BWEoV3DzoovAiawr2t2h8IcDZLLxxQUing80ma8faEk2UlYMiLQZv
# RfL37WBSALX+0oIkrZDc1lIoCb5/aMMjgTdGFA==
# SIG # End signature block
