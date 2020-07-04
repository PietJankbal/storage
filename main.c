/*
 * powershell.exe - this program only calls pwsh.exe
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

// Commands that Waves calls, the last one fails but not fatal for Waves
//powershell.exe Write-Host $PSVersionTable.PSVersion.Major $PSVersionTable.PSVersion.Minor
//powershell.exe -command &{'{0}.{1}' -f $PSVersionTable.PSVersion.Major, $PSVersionTable.PSVersion.Minor}
//powershell.exe (Get-PSDrive C).Free
//powershell.exe -noLogo -noExit  /c Register-WMIEvent -Query 'SELECT * FROM Win32_DeviceChangeEvent WHERE (EventType = 2 OR EventType = 3) GROUP WITHIN 4' -Action { [System.Console]::WriteLine('Devices Changed') }

//Compilation (64 bit):
//x86_64-w64-mingw32-gcc main.c -o powershell.exe -mconsole -municode
//Compilation (32 bit):
//i686-w64-mingw32-gcc main.c -o powershell.exe -mconsole -municode

#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>
//#include <stdio.h>
#include <process.h>

#include <windows.h>
#include <winuser.h>
#include <stdio.h>
//#include "wine/debug.h"
//#include "resources.h"

//WINE_DEFAULT_DEBUG_CHANNEL(powershell);

BOOL contains_commando(WCHAR *arg)
{
if (!wcsstr(arg, L"-c") && !wcsstr(arg, L"-C") ) return TRUE;
else
return FALSE;
}

int /*__cdecl*/ wmain(int argc, WCHAR *argv[])
{
    int i;
    WCHAR commandlineW [MAX_PATH];
    BOOL contains_command = 0; BOOL contains_noexit = 0; BOOL next_one_is_option_argument = 0; BOOL contains_special = 0;
    const WCHAR *new_args[3];
    //https://github.com/PowerShell/PowerShell/releases/download/v6.1.6/PowerShell-6.1.6-win-x64.msi
    //https://github.com/PowerShell/PowerShell/releases/download/v6.1.6/PowerShell-6.1.6-win-x86.msi
    //wine msiexec /i /media/louis/aqqa/PowerShell-6.1.6-win-x64.msi INSTALLFOLDER="C:\\Powershell6\\" /q
    //For now assume it`s installed in default path:
    const WCHAR pwshW[MAX_PATH] = L"c:\\Program Files\\PowerShell\\6\\pwsh.exe "; /* leave the space at the end! */
    const WCHAR pwsh_exeW[MAX_PATH] = L"pwsh.exe";
    WCHAR start_pwshW[MAX_PATH] = L"start pwsh.exe "; /* leave the space at the end! */


  ///////////////////////////////////////////////////////////////////////////////////



    if ( (GetFileAttributesW(L"c:\\Program Files\\PowerShell\\6\\pwsh.exe") != INVALID_FILE_ATTRIBUTES) )goto already_installed;

    MessageBoxW(NULL, (LPCWSTR)L"Looks like Powershell Core is not installed \nWill start downloading and install now\nThis will take quite some time!!!\nNo progress bar is shown!",
        (LPCWSTR)L"Message",
        MB_ICONWARNING | MB_OK
    );

    SetCurrentDirectoryW(L"c:\\windows\\temp\\");

#define MAX_LINE 512

    char url[MAX_LINE] = "https://github.com/PowerShell/PowerShell/releases/download/v6.1.6/PowerShell-6.1.6-win-x64.msi"; //
    char destination[MAX_LINE] = "PowerShell-6.1.6-win-x64.msi";
    char buffer[MAX_LINE];

    HRESULT    dl;

    typedef HRESULT (WINAPI * URLDownloadToFileA_t)(void *pCaller, LPCSTR szURL, LPCSTR szFileName, DWORD dwReserved, void * lpfnCB);
    URLDownloadToFileA_t xURLDownloadToFileA;
    xURLDownloadToFileA = (URLDownloadToFileA_t)GetProcAddress(LoadLibraryA("urlmon"), "URLDownloadToFileA");

    dl = xURLDownloadToFileA(NULL, url, destination, 0, NULL);

    printf("Downloading File From: %s, To: %s \n", url, destination);

    if(dl == S_OK)
    {
        printf("File Successfully Downloaded \n");
    }
    else 
    {
        printf( "Failed To Download File Reason: Unknown");
        return 0;
    }

    _wsystem(L"msiexec.exe /i PowerShell-6.1.6-win-x64.msi /*INSTALLFOLDER=\"C:\\Windows\\Powershell6\\\"*/ /q");

already_installed:
/////////////////////////////////////////////////////////////////////////////////////

    wprintf(L"stub:");
    for (i = 0; i < argc; i++)
       {
        wprintf(L" %ls", argv[i]);

        if (!wcsnicmp(L"-c", argv[i],2))  {contains_command++;}  /* -Command */
        if (!wcsnicmp(L"-noe", argv[i],4))  {contains_noexit++;}  /* -NoExit */

        }

    wprintf(L"\n");


    if(argc == 1) {_wsystem(start_pwshW); return 0;}

    i = 1;
    commandlineW[0] = '\0';
     /* pwsh requires a command option "/c" , powershell doesn`t e.g. "powershell -NoLogo echo $env:username" should go
    into "pwsh -NoLogo /c echo $env:username". This is troublesome initial attempt which needs lots of improvement... */

    if(wcsncmp(L"-", argv[1],1))  lstrcatW(commandlineW,L" -c ");  /*1st arg is not an option (-.....) nor -c or -command ==> prepend with /c */

    while(i<argc) {start:

                   if (!wcsnicmp(L"-ve", argv[i],3)) {i++; goto done;}    /* -Version, just skip*/

                   lstrcatW(commandlineW,argv[i]); lstrcatW(commandlineW,L" "); /* concatenate all args into one single commandline */

                   if (contains_command) goto done;

                    //FIXME HANDLE VERSION....
                   if (!wcsnicmp(L"-f", argv[i],2) || /* -File */ \
                       !wcsnicmp(L"-ps", argv[i],3) ||  /* -PSConsoleFile */ \
                       !wcsnicmp(L"-en", argv[i],3)    /* -EncodedCommand */)
                            contains_special++;





                   if (!wcsnicmp(L"-f", argv[i],2) || /* -File */ \
                       !wcsnicmp(L"-ps", argv[i],3) ||  /* -PSConsoleFile */ \
                       !wcsnicmp(L"-ve", argv[i],3) ||    /* -Version */ \
                       !wcsnicmp(L"-in", argv[i],3) ||    /* -InputFormat */ \
                       !wcsnicmp(L"-ou", argv[i],3) ||   /* -OutputFormat */ \
                       !wcsnicmp(L"-wi", argv[i],3) ||    /* -WindowStyle */ \
                       !wcsnicmp(L"-en", argv[i],3) ||   /* -EncodedCommand */ \
                       !wcsnicmp(L"-ex", argv[i],3)    /* -ExecutionPolicy */ )
                          {i++; next_one_is_option_argument = TRUE; goto start;}
                   //powershell -nologo -windowstyle normal -outputformat xml -version 1.0 echo \$env:username
                    if ( (wcsncmp(L"-", argv[i],1) && next_one_is_option_argument &&  i+1<argc && wcsncmp(L"-", argv[i+1],1)) || (!wcsncmp(L"-", argv[i],1) && i+1<argc && wcsncmp(L"-", argv[i+1],1)) ) {lstrcatW(commandlineW,L" -c "); contains_command++; next_one_is_option_argument=FALSE;}

                   done:
                   i++;
                  }

    wprintf(L"new command is %ls\n", commandlineW);

    new_args[0] = pwsh_exeW;
    new_args[1] = commandlineW;
    new_args[2] = NULL;


    /* HACK  It crashes with Invalid Handle if -noexit is present or just e.g. "powershell -log"; if powershellconsole is started it doesn`t crash... */
    if( !contains_commando(commandlineW) || ( contains_commando(commandlineW) && contains_noexit) )
   {
        if(!contains_special )
        {_wsystem(lstrcatW(start_pwshW, commandlineW)); return 0;}
    }



    _wspawnv(_P_OVERLAY, pwshW, new_args);


    return 0;
}
