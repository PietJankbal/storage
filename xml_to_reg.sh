#!/bin/sh
for i in ./*.manifest; do
cat "$i" \
| sed '/<registryKeys>/,$!d' | sed '/<\/registryKeys>/,$d' \
| sed 's/<registryKey keyName="HKEY_/[HKEY_/g'| sed 's/<registryValue name=//g' \
| sed 's/owner="false" \/>//g'| sed 's/owner="true" \/>//g' \
| sed 's/" owner="false".*>/]/g' \
| sed 's/ valueType="REG_SZ" value=/=/g'| sed 's/ owner="true" >//g' \
| sed 's/ valueType="REG_BINARY" value="/=hex:/g' \
| sed 's/ valueType="REG_DWORD" value="/=dword:/g' \
| sed 's/ valueType="REG_NONE" value=/=/g' \
| sed 's/<registryKey>//g' \
| sed 's/<\/registryKey>//g' \
| sed '/hex:.*/ s/[0-9A-F][0-9A-F]/&,/g; s/,$//' \
| sed '/hex:.*/ s/,"//' \
| sed 's/<registryKeys>/REGEDIT4/' \
| sed 's/$(runtime.system32)/c:\\\\windows\\\\system32\\/g' \
| sed 's/""=/@=/' \
| sed 's/^[ \t]*//' \
| sed '/HKEY_/ s/\\]/]/' \
| sed 's///g' \
| sed 's/\s*$//g' \
| sed '/dword:.*/ {s/.$//g}' \
| sed '/securityDescriptor name/d'  >"$i".reg #delete all lines containing "securityDescriptor name"                                                                               
done

# REG_NONE IS NOT HANDLED YET!!