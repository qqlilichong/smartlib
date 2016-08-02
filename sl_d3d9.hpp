
#ifdef WIN32

#ifndef __sl_d3d9_hpp
#define __sl_d3d9_hpp

#include <d3d9.h>
#pragma comment( lib, "d3d9" )

//////////////////////////////////////////////////////////////////////////

class CD3D9EX
{
public:
	CD3D9EX()
	{
		Direct3DCreate9Ex( D3D_SDK_VERSION, &m_d3d9 ) ;
	}

	operator IDirect3D9* ()
	{
		return m_d3d9 ;
	}

	operator IDirect3D9Ex* ()
	{
		return m_d3d9 ;
	}

	operator bool ()
	{
		return m_d3d9 != NULL ;
	}

private:
	CComQIPtr< IDirect3D9Ex >	m_d3d9 ;
};

//////////////////////////////////////////////////////////////////////////

class CD3D9Device
{
public:
	HRESULT CreateDevice( IDirect3D9Ex* pD3D9,
		HWND hWnd,
		UINT nAdapter,
		D3DPRESENT_PARAMETERS* pp = nullptr,
		UINT BackBufferWidth = 0,
		UINT BackBufferHeight = 0,
		D3DSWAPEFFECT SwapEffect = D3DSWAPEFFECT_DISCARD,
		UINT PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE,
		DWORD Flags = 0 )
	{
		if ( pD3D9 == nullptr || m_d3d9_device != nullptr )
		{
			return S_FALSE ;
		}

		D3DPRESENT_PARAMETERS d3dpp = { 0 } ;

		d3dpp.Windowed = TRUE ;
		d3dpp.BackBufferWidth = BackBufferWidth ;
		d3dpp.BackBufferHeight = BackBufferHeight ;
		d3dpp.BackBufferCount = 0 ;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN ;

		d3dpp.SwapEffect = SwapEffect ;
		d3dpp.PresentationInterval = PresentationInterval ;
		d3dpp.Flags = Flags ;
		
		if ( pD3D9->CreateDeviceEx( nAdapter, D3DDEVTYPE_HAL, hWnd,
			D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
			&d3dpp, NULL, &m_d3d9_device ) != S_OK )
		{
			return S_FALSE ;
		}
		
		if ( pp != nullptr )
		{
			*pp = d3dpp ;
		}

		return S_OK ;
	}
	
	HRESULT UpdateBackSurface( LPDIRECT3DSURFACE9 pSurface,
		const LPRECT pDestRect = NULL, const LPRECT pSrcRect = NULL,
		D3DTEXTUREFILTERTYPE Filter = D3DTEXF_LINEAR )
	{
		CComQIPtr< IDirect3DSurface9 > backsurface ;
		if ( m_d3d9_device->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &backsurface ) != S_OK )
		{
			return S_FALSE ;
		}
		
		return m_d3d9_device->StretchRect( pSurface, pSrcRect, backsurface, pDestRect, Filter ) ;
	}
	
	HRESULT CreateOffscreenSurface( UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface9** pp )
	{
		return m_d3d9_device->CreateOffscreenPlainSurface( Width, Height, Format, D3DPOOL_DEFAULT, pp, NULL ) ;
	}
	
	HRESULT Present( const LPRECT pDestRect = NULL, const LPRECT pSrcRect = NULL, HWND hWnd = NULL )
	{
		return m_d3d9_device->Present( pSrcRect, pDestRect, hWnd, NULL ) ;
	}
	
	operator IDirect3DDevice9* ()
	{
		return m_d3d9_device ;
	}
	
	operator IDirect3DDevice9Ex* ()
	{
		return m_d3d9_device ;
	}

	operator bool ()
	{
		return m_d3d9_device != NULL ;
	}

private:
	CComQIPtr< IDirect3DDevice9Ex >	m_d3d9_device ;
};

//////////////////////////////////////////////////////////////////////////

#ifdef __sl_ffmpeg_hpp

inline D3DFORMAT sl_map_pixelfmt( int idx )
{
	static map< AVPixelFormat, D3DFORMAT > map_pix_fmt ;

	try
	{
		if ( map_pix_fmt.empty() )
		{
			map_pix_fmt[ AV_PIX_FMT_YUV420P ] = 
				(D3DFORMAT)MAKEFOURCC( 'Y', 'V', '1', '2' ) ;
			
			map_pix_fmt[ AV_PIX_FMT_YUVJ420P ] = 
				(D3DFORMAT)MAKEFOURCC( 'Y', 'V', '1', '2' ) ;

			map_pix_fmt[ AV_PIX_FMT_NV12 ] = 
				(D3DFORMAT)MAKEFOURCC( 'N', 'V', '1', '2' ) ;
		}
		
		return map_pix_fmt.at( (AVPixelFormat)idx ) ;
	}

	catch ( ... )
	{
		return D3DFMT_UNKNOWN ;
	}
	
	return map_pix_fmt[ (AVPixelFormat)idx ] ;
}

inline CComQIPtr< IDirect3DSurface9 > sl_avframe2surface( CD3D9Device& render, AVFrame* frame )
{
	bool done = false ;
	CComQIPtr< IDirect3DSurface9 > surface ;

	try
	{
		if ( render.CreateOffscreenSurface(
			frame->width,
			frame->height,
			sl_map_pixelfmt( frame->format ),
			&surface ) != 0 )
		{
			throw -1 ;
		}

		D3DLOCKED_RECT lr = { 0 } ;
		if ( surface->LockRect( &lr, nullptr, 0 ) == 0 )
		{
			bool sw_uv = false ;
			uint8_t* dst_data[ 4 ] = { 0 } ;
			int dst_linesize[ 4 ] = { 0 } ;
			
			switch ( frame->format )
			{
				case AV_PIX_FMT_YUV420P :
				case AV_PIX_FMT_YUVJ420P :
					dst_linesize[ 0 ] = lr.Pitch ;
					dst_linesize[ 1 ] = dst_linesize[ 2 ] = dst_linesize[ 0 ] / 2 ;
					sw_uv = true ;
					break ;

				case AV_PIX_FMT_NV12 :
					dst_linesize[ 0 ] = dst_linesize[ 1 ] = lr.Pitch ;
					break ;
			}
			
			if ( dst_linesize[ 0 ] )
			{
				av_image_fill_pointers( dst_data, (AVPixelFormat)frame->format,
					frame->height, (uint8_t*)lr.pBits, dst_linesize ) ;
			}

			if ( dst_data[ 0 ] )
			{
				if ( sw_uv )
				{
					std::swap( dst_data[ 1 ], dst_data[ 2 ] ) ;
				}

				av_image_copy( dst_data, dst_linesize,
					(const uint8_t**)frame->data, frame->linesize,
					(AVPixelFormat)frame->format, frame->width, frame->height ) ;

				done = true ;
			}
			
			surface->UnlockRect() ;
		}
	}

	catch ( ... )
	{

	}

	if ( !done )
	{
		surface.Release() ;
	}

	return surface ;
}

#endif // __sl_ffmpeg_hpp

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_d3d9_hpp

#endif // #ifdef WIN32
