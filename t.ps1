    #$Xml = [xml](Get-Content -Path "$env:SystemRoot\\syswow64\\$manifest")
    #$manifest = x86_microsoft-windows-msmpeg2vdec_31bf3856ad364e35_6.1.7601.23403_none_9391e9c22f5ac446.manifest

 

   $Xml = [xml](Get-Content -Path "$env:SystemRoot\\syswow64\\x86_microsoft-windows-msmpeg2vdec_31bf3856ad364e35_6.1.7601.23403_none_9391e9c22f5ac446.manifest")

#thanks guy from freenode webchat channel powershell!
foreach ($key in $Xml.assembly.registryKeys.registryKey) {
    $path = 'Registry::{0}' -f $key.keyName
    if (-not (Test-Path -Path $path)) {
        New-Item -Path $path -ItemType Key -Force
    }
    foreach ($value in $key.registryValue) {
        $propertyType = switch ($value.valueType) {
            'REG_SZ' { 'String' }
        }
        New-ItemProperty -Path $path -Name $value.Name -Value $value.Value -PropertyType $propertyType -Force
    }
}


#    ForEach ($key in $Xml.assembly.registrykeys.registrykey)
#    {
#    $PropArgs = @{
#        'Path'         = $key.keyName
#        'Name'         = $key.registryValue.name


#        'Value'        = $key.registryValue.value

#        'PropertyType' = $key.registryValue.valueType
        #'Force'        = "True"


#    }
#     Write-Host @PropArgs    #New-ItemProperty







#recurse:
#New-Item "HKLM:\Software\test\test\test\test" -Force | New-ItemProperty -Name "TestEntry" -Value "TestValue" -PropertyType String
 #   }