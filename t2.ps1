 $\(runtime.system32\) = "$env:SystemRoot\\"

   $Xml = [xml](Get-Content -Path "$env:SystemRoot\\syswow64\\x86_microsoft-windows-msmpeg2vdec_31bf3856ad364e35_6.1.7601.23403_none_9391e9c22f5ac446.manifest")
#Write the regkeys from manifest file
#thanks some guy from freenode webchat channel powershell for great help!
foreach ($key in $Xml.assembly.registryKeys.registryKey) {
    $path = 'Registry::{0}' -f $key.keyName
    if (-not (Test-Path -Path $path)) {
        New-Item -Path $path -ItemType Key -Force
    }

    foreach ($value in $key.registryValue) {
        $propertyType = switch ($value.valueType) {
            'REG_SZ'         { 'String' }
            'REG_BINARY'     { 'Binary' }
            'REG_DWORD'      { 'DWORD'  }
	    'REG_EXPAND_SZ'  { 'ExpandString' } 
	    'REG_MULTI_SZ'   { 'MultiString'  } 
	    'REG_QWORD'      { 'QWord' }
            'REG_NONE'       { '' } 
        }
        $Regname = switch ($value.Name) {
            '' { ‘(Default)’ }
            default { $value.Name }
        }
        #If ($propertyType -eq "Binary") { $value.Value = [System.Text.Encoding]::Unicode.GetBytes($value.Value + "000") ; $value.Value.Replace(" ",",")}
        #https://stackoverflow.com/questions/54543075/how-to-convert-a-hash-string-to-byte-array-in-powershell
        If ($propertyType -eq "Binary") {$hashByteArray = [byte[]] ($value.Value -replace '..', '0x$&,' -split ',' -ne '');New-ItemProperty -Path $path -Name $Regname -Value $hashByteArray  -PropertyType $propertyType -Force}
        else{

        New-ItemProperty -Path $path -Name $Regname -Value $value.Value -PropertyType $propertyType -Force}
    }
}
