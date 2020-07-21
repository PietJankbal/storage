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
 *

 * Commands that Waves calls, the last one fails but not fatal for Waves
 * powershell.exe Write-Host $PSVersionTable.PSVersion.Major $PSVersionTable.PSVersion.Minor
 * powershell.exe -command &{'{0}.{1}' -f $PSVersionTable.PSVersion.Major, $PSVersionTable.PSVersion.Minor}
 * powershell.exe (Get-PSDrive C).Free
 * powershell.exe -noLogo -noExit  /c Register-WMIEvent -Query 'SELECT * FROM Win32_DeviceChangeEvent WHERE (EventType = 2 OR EventType = 3) GROUP WITHIN 4' -Action { [System.Console]::WriteLine('Devices Changed') }

 * Downloads
 * https://github.com/PowerShell/PowerShell/releases/download/v6.1.6/PowerShell-6.1.6-win-x64.msi
 * https://github.com/PowerShell/PowerShell/releases/download/v6.1.6/PowerShell-6.1.6-win-x86.msi
 * wine msiexec /i /media/louis/aqqa/PowerShell-6.1.6-win-x64.msi INSTALLFOLDER="C:\\Powershell6\\" /q

 * Compilation (64 bit):
 * x86_64-w64-mingw32-gcc main.c -o powershell.exe -mconsole -municode -lurlmon ???--builtin???
 * Compilation (32 bit):
 * i686-w64-mingw32-gcc main.c -o powershell.exe -mconsole -municode -lurlmon
*/

//#include "wine/unicode.h"

//#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>

//#include <process.h>


#include <windows.h>
#include <winuser.h>
#include <winternl.h>
#include <winbase.h>
#include <stdio.h>

#include <urlmon.h>

#undef __WINESRC__
#include <tchar.h>
#include "msvcrt/corecrt.h"

//#if (__STDC_VERSION__ >= 199901L)
#include <stdint.h>
//#endif

#include "wine/heap.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(powershell);

intptr_t __cdecl _wspawnv(int,const wchar_t*,const wchar_t* const *);
_ACRTIMP int WINAPIV wprintf(const wchar_t*,...);


#define MAX_LINE 512

const WCHAR _cW[] = {'-','c','\0'};
//const WCHAR spaceW_cW_spaceW[] = {' ','-','c',' ','\0'};
const WCHAR _CW[] = {'-','C','\0'};
const WCHAR newlineW[] = {'\n','\0'};
const WCHAR dashW[] = {'-','\0'};
const WCHAR stubW[] = {'s','t','u','b',':','\0'};
const WCHAR fmt_lsW[] = {' ','%','l','s','\0'};
const WCHAR new_command_is[] = {'n','e','w',' ','c','o','m','m','a','n','d',' ','i','s',' ','%','l','s','\n','\0'};
static const WCHAR pwdirW[] = {'\\','P','o','w','e','r','S','h','e','l','l','\\','6','\\','p','w','s','h','.','e','x','e',' ','\0'};
const WCHAR Program6432W[] = {'%','P','r','o','g','r','a','m','W','6','4','3','2','%','\0'};
const WCHAR spaceW_cW_spaceW[] = {' ','-','c',' ','\0'};

// https://creativeandcritical.net/str-replace-c#repl_wcs   :Replaces in the string str all the occurrences of the source string from with the destination string to.           Re


wchar_t *repl_wcs(const wchar_t *str, const wchar_t *from, const wchar_t *to) {

	/* Adjust each of the below values to suit your needs. */

	/* Increment positions cache size initially by this number. */
	size_t cache_sz_inc = 4;
	/* Thereafter, each time capacity needs to be increased,
	 * multiply the increment by this factor. */
	const size_t cache_sz_inc_factor = 16;
	/* But never increment capacity by more than this number. */
	const size_t cache_sz_inc_max = 1048576;

	wchar_t *pret, *ret = NULL;
	const wchar_t *pstr2, *pstr = str;
	size_t i, count = 0;
	//#if (__STDC_VERSION__ >= 199901L)
	//uintptr_t *pos_cache_tmp, *pos_cache = NULL;
	//#else
	ptrdiff_t *pos_cache_tmp, *pos_cache = NULL;
	//#endif
	size_t cache_sz = 0;
	size_t cpylen, orglen, retlen, tolen, fromlen = lstrlenW(from);

	/* Find all matches and cache their positions. */
	while ((pstr2 = strstrW(pstr, from)) != NULL) {
		count++; 

		/* Increase the cache size when necessary. */
		if (cache_sz < count) {
			cache_sz += cache_sz_inc;
			pos_cache_tmp = (ptrdiff_t *)heap_realloc(pos_cache, sizeof(*pos_cache) * cache_sz);
			if (pos_cache_tmp == NULL) {
				goto end_repl_wcs;
			} else pos_cache = pos_cache_tmp;
			cache_sz_inc *= cache_sz_inc_factor;
			if (cache_sz_inc > cache_sz_inc_max) {
				cache_sz_inc = cache_sz_inc_max;
			}
		}

		pos_cache[count-1] = pstr2 - str ; 
		pstr = pstr2 + fromlen ; 
	}

	orglen = pstr - str + lstrlenW(pstr);

	/* Allocate memory for the post-replacement string. */
	if (count > 0) {
		tolen = lstrlenW(to);
		retlen = orglen + (tolen - fromlen) * count;
	} else	retlen = orglen;
	ret = (wchar_t *)heap_alloc((retlen + 1) * sizeof(wchar_t));
	if (ret == NULL) {
		goto end_repl_wcs;
	}

	if (count == 0) {
		/* If no matches, then just duplicate the string. */
		lstrcpyW(ret, str);
	} else {
		/* Otherwise, duplicate the string whilst performing
		 * the replacements using the position cache. */
		pret = ret;

		wmemcpy(pret, str, pos_cache[0]);

//		memcpy(pret, str,sizeof(wchar_t)  + sizeof(wchar_t)*  pos_cache[0]);
		pret += pos_cache[0];
		for (i = 0; i < count; i++) {
			wmemcpy(pret, to, tolen); 
			pret += tolen;
			pstr = str + pos_cache[i] + fromlen; 
			cpylen = (i == count-1 ? orglen : pos_cache[i+1]) - pos_cache[i] - fromlen;
			wmemcpy(pret, pstr, cpylen); 
			pret += cpylen; 
		}
		ret[retlen] = L'\0';printf("\033[1;35m"); wprintf(new_command_is, ret); printf("\033[0m;");
	}

end_repl_wcs:
	/* Free the cache and return the post-replacement string,
	 * which will be NULL in the event of an error. */
	heap_free(pos_cache);

    //    printf("\033[1;35m"); wprintf(new_command_is, ret); printf("\033[0m;");

	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
wchar_t * appendstr(wchar_t * string, const wchar_t * append) {
	wchar_t * newString = heap_alloc((lstrlenW(string) + lstrlenW(append) + 1)*sizeof(wchar_t));

	swprintf(newString, "%ls%ls", string, append);
	heap_free(string);
	return newString;
}

wchar_t * strtokk(wchar_t * string, const wchar_t * strf) {
	static wchar_t * ptr;
	static wchar_t * ptr2;

	if (!*strf) return string;
	if (string) ptr = string;
	else {
		if (!ptr2) return ptr2;
		ptr = ptr2 + lstrlenW(strf)*sizeof(wchar_t);
	}

	if (ptr) {
		ptr2 = strstrW(ptr, strf);
		if (ptr2) memset(ptr2, 0, lstrlenW(strf)*sizeof(wchar_t));
	}
	return ptr;
}


wchar_t * strrep(const wchar_t * cadena, const wchar_t * strf, const wchar_t * strr) {
	wchar_t * string;
	wchar_t * ptr;
	wchar_t * strrep;

	string = (wchar_t *)heap_alloc((lstrlenW(cadena) + 1)*sizeof(wchar_t));
	swprintf(string, "%ls", cadena);
	if (!*strf) return string;
	ptr = strtokk(string, strf);
	strrep = heap_alloc((lstrlenW(ptr) + 1)*sizeof(wchar_t));
	memset(strrep, 0, (lstrlenW(ptr) + 1)*sizeof(wchar_t));
	while (ptr) {
		strrep = appendstr(strrep, ptr);
		ptr = strtokk(NULL, strf);
		if (ptr) strrep = appendstr(strrep, strr);
	}
	heap_free(string);
        printf("\033[1;35m"); wprintf(new_command_is, strrep); printf("\033[0m;");
	return strrep;
}

///////////////////////////////////////////////////////////////////////////////////////////////
wchar_t * str_replace(wchar_t * text,wchar_t * rep, wchar_t * repw){//text -> to replace in it | rep -> replace | repw -> replace with
    int replen = lstrlenW(rep),repwlen = lstrlenW(repw),count;//some constant variables
    for(int i=0;i<lstrlenW(text);i++){//search for the first wchar_tacter from rep in text
        if(text[i] == rep[0]){//if it found it
            count = 1;//start searching from the next wchar_tacter to avoid repetition
            for(int j=1;j<replen;j++){
                if(text[i+j] == rep[j]){//see if the next wchar_tacter in text is the same as the next in the rep if not break
                    count++;
                }else{
                    break;
                }
            }
            if(count == replen){//if count equals to the lenght of the rep then we found the word that we want to replace in the text
                if(replen < repwlen){
                    for(int l = lstrlenW(text);l>i;l--){//cuz repwlen greater than replen we need to shift wchar_tacters to the right to make space for the replacement to fit
                        text[l+repwlen-replen] = text[l];//shift by repwlen-replen
                    }
                }
                if(replen > repwlen){
                    for(int l=i+replen-repwlen;l<lstrlenW(text);l++){//cuz replen greater than repwlen we need to shift the wchar_tacters to the left
                        text[l-(replen-repwlen)] = text[l];//shift by replen-repwlen
                    }
                    text[lstrlenW(text)-(replen-repwlen)] = '\0';//get rid of the last unwanted wchar_tacters
                }
                for(int l=0;l<repwlen;l++){//replace rep with repwlen
                    text[i+l] = repw[l];
                }
                if(replen != repwlen){
                    i+=repwlen-1;//pass to the next wchar_tacter | try text "y" ,rep "y",repw "yy" without this line to understand
                }
            }
        }
    }
    return text;
}


//////////////////////////////////////////////////////////////////////////////////
void strreplace(wchar_t *string, const wchar_t *find, const wchar_t *replaceWith){
    if(strstrW(string, replaceWith) != NULL){
        wchar_t *temporaryString = heap_alloc ( (lstrlenW(strstrW(string, find) + lstrlenW(find)) + 1) *sizeof(wchar_t));
        lstrcpyW(temporaryString, strstrW(string, find) + lstrlenW(find));    //Create a string with what's after the replaced part
//        *strstrW(string, find) = '\0';    //Take away the part to replace and the part after it in the initial string
        lstrcpyW(strstrW(string, find),'\0');    //Take away the part to replace and the part after it in the initial string

        lstrcatW(string, replaceWith);    //Concat the first part of the string with the part to replace with
        lstrcatW(string, temporaryString);    //Concat the first part of the string with the part after the replaced part
        heap_free(temporaryString);    //Free the memory to avoid memory leaks
    }
}










BOOL contains_commando(WCHAR *arg)
{
    if ( strstrW(arg, _cW) || strstrW(arg, _CW) )
        return TRUE;
    else
        return FALSE;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    int i;
    WCHAR cur_dirW[MAX_PATH];
    WCHAR installW[MAX_PATH];

    WCHAR commandlineW [MAX_PATH];
    BOOL contains_command = 0; BOOL contains_noexit = 0; BOOL next_one_is_option_argument = 0; BOOL needs_work_around_crash = 0;
    const WCHAR *new_args[3];

    /* For now assume it`s installed in default path: */
    WCHAR pwsh_pathW[MAX_PATH];

    ExpandEnvironmentStringsW(Program6432W, pwsh_pathW, MAX_PATH+1);
    //wprintf(fmt_lsW, pwsh_pathW);
    lstrcatW(pwsh_pathW, pwdirW); /* leave the space at the end! */
    WCHAR pwsh_exeW[] = {'p','w','s','h','.','e','x','e','\0'};
    WCHAR start_pwshW[] = {'s','t','a','r','t',' ','p','w','s','h','.','e','x','e',' ','\0'}; /* leave the space at the end! */

    char url[MAX_LINE] = "https://github.com/PowerShell/PowerShell/releases/download/v6.1.6/PowerShell-6.1.6-win-x64.msi"; //
    char destination[MAX_LINE] = "PowerShell-6.1.6-win-x64.msi";
  
    if ( (GetFileAttributesW(pwsh_pathW) != INVALID_FILE_ATTRIBUTES) )
        goto already_installed;

    /* Download */
    MessageBoxA(NULL, "    Looks like Powershell Core is not installed \nWill start downloading and install now\n \
    This will take quite some time!!!\nNo progress bar is shown!", "Message", MB_ICONWARNING | MB_OK);

    GetCurrentDirectoryW(MAX_PATH+1, cur_dirW);
    WCHAR tempW[] = {'c',':','\\','w','i','n','d','o','w','s','\\','t','e','m','p','\\','\0'};
    SetCurrentDirectoryW(tempW);
    printf("Downloading File From: %s, To: %s \n", url, destination);

    if( URLDownloadToFileA(NULL, url, destination, 0, NULL) != S_OK )
        goto failed;
    else
        printf("File Successfully Downloaded \n");

    system("msiexec.exe /i PowerShell-6.1.6-win-x64.msi /*INSTALLFOLDER=\"C:\\Windows\\Powershell6\\\"*/ /q");
    SetCurrentDirectoryW(cur_dirW);

already_installed:

    if(argc == 1)
    {
        _wsystem(start_pwshW);
        return 0;
    }

    WCHAR _noe[] = {'-','n','o','e','\0'};
    WCHAR spaceW[] = {' ','\0'};

    // wprintf(stubW); //WINE_FIXME("stub:");//  
    for (i = 0; i < argc; i++)
    {
      //  wprintf(fmt_lsW, argv[i]);
        if (!strncmpiW(_cW, argv[i],2))     contains_command++;  /* -Command */
        if (!strncmpiW(_noe, argv[i],4))   contains_noexit++;  /* -NoExit */
    }
   // wprintf(newlineW);

    i = 1;
    commandlineW[0] = '\0';
     /* pwsh requires a command option "-c" , powershell doesn`t e.g. "powershell -NoLogo echo $env:username" should go
    into "pwsh -NoLogo -c echo $env:username". This is troublesome initial attempt which needs lots of improvement... */

    if(strncmpiW(dashW, argv[1],1))  {lstrcatW(commandlineW, spaceW_cW_spaceW);  contains_command++;}/*1st arg is not an option (-.....) nor -c or -command ==> prepend with /c */



    WCHAR _f[] = {'-','f','\0'};
    WCHAR _ps[] = {'-','p','s','\0'};
    WCHAR _en[] = {'-','e','n','\0'};

    WCHAR _ve[] = {'-','v','e','\0'};
    WCHAR _in[] = {'-','i','n','\0'};
    WCHAR _ou[] = {'-','o','u','\0'};
    WCHAR _wi[] = {'-','w','i','\0'};
    WCHAR _ex[] = {'-','e','x','\0'};





    while(i<argc)
    {
        start:
        if (!strncmpiW(_ve, argv[i],3)) {i++; goto done;}    /* -Version, just skip*/

        lstrcatW(commandlineW,argv[i]); lstrcatW(commandlineW, spaceW); /* concatenate all args into one single commandline */

        if (contains_command) goto done;

        /* For now start powershellconsole if options below are present because otherwise it crashes; possibly due to bug in wine (?????) */
        if ( !strncmpiW(_f, argv[i],2) || /* -File */ \
             !strncmpiW(_ps, argv[i],3) ||  /* -PSConsoleFile */ \
             !strncmpiW(_en, argv[i],3)    /* -EncodedCommand */)
                 needs_work_around_crash++;

        if ( !strncmpiW(_f, argv[i],2) || /* -File */ \
             !strncmpiW(_ps, argv[i],3) ||  /* -PSConsoleFile */ \
             !strncmpiW(_ve, argv[i],3) ||    /* -Version */ \
             !strncmpiW(_in, argv[i],3) ||    /* -InputFormat */ \
             !strncmpiW(_ou, argv[i],3) ||   /* -OutputFormat */ \
             !strncmpiW(_wi, argv[i],3) ||    /* -WindowStyle */ \
             !strncmpiW(_en, argv[i],3) ||   /* -EncodedCommand */ \
             !strncmpiW(_ex, argv[i],3)    /* -ExecutionPolicy */ )
             {
                 i++;
                 next_one_is_option_argument = TRUE;
                 goto start;
             }
             //powershell -nologo -windowstyle normal -outputformat xml -version 1.0 echo \$env:username
             if ( (strncmpiW(dashW, argv[i],1) && next_one_is_option_argument &&  i+1<argc && strncmpiW(dashW, argv[i+1],1)) || (!strncmpiW(dashW, argv[i],1) && i+1<argc && strncmpiW(dashW, argv[i+1],1)) ) {lstrcatW(commandlineW,spaceW_cW_spaceW); contains_command++; next_one_is_option_argument=FALSE;}

        done:
        i++;
    }

//    printf("\033[1;35m"); wprintf(new_command_is, commandlineW); printf("\033[0m;");

    char arr[][MAX_PATH] = { "garbage", "ps",
                             "asr", "ps"
                           };

      WCHAR bufW [MAX_PATH] ;
   //   bufW[0] =  '\0';
      lstrcpyW(bufW, commandlineW);

      ANSI_STRING from, to;
      UNICODE_STRING fromW, toW;

      for(i=0; i < ARRAY_SIZE(arr);i+=2){
      RtlInitAnsiString(&from, arr[i]);
      RtlInitAnsiString(&to, arr[i+1]);
      RtlAnsiStringToUnicodeString(&fromW, &from, TRUE);
      RtlAnsiStringToUnicodeString(&toW, &to, TRUE);

 
 //     lstrcpyW(commandlineW, repl_wcs(bufW, fromW.Buffer, toW.Buffer));

     
      lstrcpyW( bufW, repl_wcs(bufW, fromW.Buffer, toW.Buffer) );

//    printf("\033[1;35m"); wprintf(new_command_is, bufW); printf("\033[0m;");
      }

      RtlFreeUnicodeString(&fromW); RtlFreeUnicodeString(&toW);


    new_args[0] = pwsh_exeW;
    new_args[1] = bufW;//commandlineW;//bufW;
    new_args[2] = NULL;

  //  printf("\033[1;35m"); fwprintf(&stderr,new_command_is, bufW); printf("\033[0m;");


    /* HACK  It crashes with Invalid Handle if -noexit is present or just e.g. "powershell -nologo"; if powershellconsole is started it doesn`t crash... */
    if( !contains_commando(commandlineW) || ( contains_commando(commandlineW) && contains_noexit) )
    {
        if(!needs_work_around_crash)
        {
            _wsystem(lstrcatW(start_pwshW, commandlineW));
            return 0;
        }
    }

    _wspawnv(2/*_P_OVERLAY*/, pwsh_pathW, new_args);
    return 0;

failed:
    printf("Something went wrong :( \n");
    return 1;
}
