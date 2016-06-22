
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

#define SLM_MAKESOFTWAREEXCEPTION( Severity, Facility, Exception )	\
	( (DWORD)(\
	/* Severity code	*/ ( Severity		)		|\
	/* MS(0) or Cust(1)	*/ ( 1 << 29		)		|\
	/* Reserved(0)		*/ ( 0 << 28		)		|\
	/* Facility code	*/ ( Facility << 16 )		|\
	/* Exception code	*/ ( Exception << 0 )		)	)

//////////////////////////////////////////////////////////////////////////

#define	SLM_BEGINTHREAD( psa, cbStackSize, pfnStartAddr, pvParam, dwCreateFlags, pdwThreadId )	\
	( (HANDLE)_beginthreadex(\
	(void*)								psa,			\
	(unsigned)							cbStackSize,	\
	(unsigned int (__stdcall *)(void *))pfnStartAddr,	\
	(void*)					pvParam,					\
	(unsigned)				dwCreateFlags,				\
	(unsigned*)				pdwThreadId					)	)

//////////////////////////////////////////////////////////////////////////

#define SLM_MAKEFOURCC(ch0, ch1, ch2, ch3)	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

//////////////////////////////////////////////////////////////////////////

inline void CSmartHandleDeleter_CloseHandle( HANDLE& handle )
{
	CloseHandle( handle ) ;
}

inline void CSmartHandleDeleter_FreeLibrary( HMODULE& handle )
{
	FreeLibrary( handle ) ;
}

inline void CSmartHandleDeleter_LocalFree( HLOCAL& handle )
{
	LocalFree( handle ) ;
}

typedef CSmartHandle< HANDLE,	INVALID_HANDLE_VALUE,	CSmartHandleDeleter_CloseHandle >	CAutoFile	;
typedef CSmartHandle< HANDLE,	NULL,					CSmartHandleDeleter_CloseHandle >	CAutoHandle	;
typedef CSmartHandle< HMODULE,	NULL,					CSmartHandleDeleter_FreeLibrary >	CAutoModule	;
typedef CSmartHandle< HLOCAL,	NULL,					CSmartHandleDeleter_LocalFree	>	CAutoLocal	;

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_windows_hpp

#endif // #ifdef WIN32
