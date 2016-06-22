
#ifdef WIN32

#ifndef __sl_ui_hpp
#define __sl_ui_hpp

//////////////////////////////////////////////////////////////////////////

#define SLM_WINDOW_PROC_BEGIN( x )																						\
		public:																											\
		static const wchar_t* GetWindowClassName() { return x ; }														\
		private:																										\
		virtual LRESULT MessageProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {								\
		switch( uMsg ) {
#define SLM_WINDOW_PROC_MSGHANDLE(  m, p )		case ( m )	: return HANDLE_##m( hWnd, wParam, lParam, p )	;
#define SLM_WINDOW_PROC_MSGPRIVATE( m, p )		case ( m )	: return p( hWnd, uMsg, wParam, lParam )		;
#define SLM_WINDOW_PROC_END()					case WM_NULL : break ; } return DefWindowProc( hWnd, uMsg, wParam, lParam ) ; }

//////////////////////////////////////////////////////////////////////////

__interface ISL_WINDOW_PROC
{
	virtual LRESULT MessageProc( HWND, UINT, WPARAM, LPARAM ) = 0 ;
};

inline LRESULT CALLBACK SL_WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
		case WM_NCCREATE :
		{
			auto pCS = (LPCREATESTRUCT)lParam ;
			SetWindowLongPtr( hWnd, GWLP_USERDATA, (ULONG_PTR)(ISL_WINDOW_PROC*)pCS->lpCreateParams ) ;
			break ;
		}
	
		case WM_DESTROY:
		{
			PostQuitMessage( 0 ) ;
			break ;
		}
		
		default :
		{
			auto pCallback = (ISL_WINDOW_PROC*)GetWindowLongPtr( hWnd, GWLP_USERDATA ) ;
			if ( pCallback != nullptr )
			{
				return pCallback->MessageProc( hWnd, uMsg, wParam, lParam ) ;
			}
			
			break ;
		}
	}
	
	return DefWindowProc( hWnd, uMsg, wParam, lParam ) ;
}

//////////////////////////////////////////////////////////////////////////

template< class T >
class CSimWindow : public ISL_WINDOW_PROC
{
public:
	virtual ~CSimWindow()
	{
		CloseWin() ;
	}

public:
	static BOOL Register()
	{
		WNDCLASSEX wcx = { 0 } ;
		wcx.cbSize = sizeof( wcx ) ;
		wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS ;
		wcx.lpfnWndProc = SL_WindowProc ;
		wcx.hCursor = LoadCursor( NULL, IDC_ARROW ) ;
		wcx.hIcon = LoadIcon( NULL, IDI_APPLICATION ) ;
		wcx.hbrBackground = GetStockBrush( BLACK_BRUSH ) ;
		wcx.lpszClassName = T::GetWindowClassName() ;
		return RegisterClassEx( &wcx ) ;
	}
	
	BOOL CreateWin( RECT rc, const wchar_t* pszTitle,
		DWORD dwStyle = WS_POPUP, HWND hParentWnd = NULL )
	{
		if ( IsWindow( m_hWnd ) )
		{
			return FALSE ;
		}
		
		m_hWnd = CreateWindow( T::GetWindowClassName(), pszTitle,
			dwStyle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			hParentWnd, NULL, NULL, (ISL_WINDOW_PROC*)this ) ;
		
		return IsWindow( m_hWnd ) ;
	}

	BOOL CloseWin()
	{
		if ( IsWindow( m_hWnd ) )
		{
			SendMessage( m_hWnd, WM_CLOSE, 0, 0 ) ;
			m_hWnd = NULL ;
		}

		return TRUE ;
	}

	HWND GetHWND()
	{
		return m_hWnd ;
	}

private:
	SLM_WINDOW_PROC_BEGIN( L"CSimWindow" )
	SLM_WINDOW_PROC_END()

protected:
	HWND	m_hWnd = NULL ;
};

//////////////////////////////////////////////////////////////////////////

inline LONG RECT_WIDTH( RECT& rc )
{
	return rc.right - rc.left ;
}

inline LONG RECT_HEIGHT( RECT& rc )
{
	return rc.bottom - rc.top ;
}

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_ui_hpp

#endif // #ifdef WIN32
