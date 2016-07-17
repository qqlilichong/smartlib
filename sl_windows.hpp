
#ifdef WIN32

#ifndef __sl_windows_hpp
#define __sl_windows_hpp

#define WIN32_LEAN_AND_MEAN
#include <sdkddkver.h>
#include <windows.h>
#include <windowsx.h>
#include <process.h>
#include <atlbase.h>
#include <shlwapi.h>
#pragma comment( lib, "shlwapi.lib" )

//////////////////////////////////////////////////////////////////////////

#define SLM_MAKEFOURCC(ch0, ch1, ch2, ch3)	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

//////////////////////////////////////////////////////////////////////////

DECLARE_SLSH( CAutoFile,	HANDLE,		INVALID_HANDLE_VALUE,	CloseHandle( handle )	) ;
DECLARE_SLSH( CAutoHandle,	HANDLE,		NULL,					CloseHandle( handle )	) ;
DECLARE_SLSH( CAutoModule,	HMODULE,	NULL,					FreeLibrary( handle )	) ;
DECLARE_SLSH( CAutoLocal,	HLOCAL,		NULL,					LocalFree( handle )		) ;

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_windows_hpp

#endif // #ifdef WIN32
