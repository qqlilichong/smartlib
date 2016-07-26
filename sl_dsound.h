
#pragma once

#include <mmreg.h>
#include <dsound.h>
#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "dxguid.lib" )

//////////////////////////////////////////////////////////////////////////

class CDSound
{
public:
	CDSound( DWORD dwLevel = DSSCL_PRIORITY, HWND hWnd = GetDesktopWindow() )
	{
		if ( DirectSoundCreate( NULL, &m_dsound, NULL ) == S_OK )
		{
			m_dsound->SetCooperativeLevel( hWnd, dwLevel ) ;
		}
	}

	operator IDirectSound* ()
	{
		return m_dsound ;
	}

	operator bool ()
	{
		return m_dsound != nullptr ;
	}

private:
	CComQIPtr< IDirectSound,
		&IID_IDirectSound > m_dsound ;
};

//////////////////////////////////////////////////////////////////////////

class CDSoundBuffer
{
public:
	HRESULT CreateBuffer( IDirectSound* pDS, DWORD nSamplesPerSec, WORD nChannels, WORD wBitsPerSample )
	{
		if ( pDS == nullptr || m_dsound_buffer != nullptr )
		{
			return S_FALSE ;
		}

		WAVEFORMATEX wfm = { 0 } ;
		wfm.wFormatTag = WAVE_FORMAT_PCM ;
		wfm.nChannels = nChannels ;
		wfm.nSamplesPerSec = nSamplesPerSec ;
		wfm.wBitsPerSample = wBitsPerSample ;
		wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels ;
		wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign ;

		DSBUFFERDESC desc = { 0 } ;
		desc.dwSize = sizeof( desc ) ;
		desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPAN | DSBCAPS_CTRLPOSITIONNOTIFY ;
		desc.dwBufferBytes = wfm.nAvgBytesPerSec / 2 ;
		desc.lpwfxFormat = &wfm ;
		
		if ( pDS->CreateSoundBuffer( &desc, &m_dsound_buffer, NULL ) != S_OK )
		{
			return S_FALSE ;
		}

		m_notify_mid = CreateEvent( NULL, FALSE, FALSE, NULL ) ;
		if ( !m_notify_mid )
		{
			return S_FALSE ;
		}

		m_notify_end = CreateEvent( NULL, FALSE, FALSE, NULL ) ;
		if ( !m_notify_end )
		{
			return S_FALSE ;
		}
		
		CComQIPtr< IDirectSoundNotify, 
			&IID_IDirectSoundNotify > dsNotify ;
		if ( m_dsound_buffer->QueryInterface( IID_IDirectSoundNotify, (void**)&dsNotify ) != S_OK )
		{
			return S_FALSE ;
		}
		
		DSBPOSITIONNOTIFY dsb[ 2 ] ;
		dsb[ 0 ].dwOffset = ( desc.dwBufferBytes / 2 ) - 1 ;
		dsb[ 0 ].hEventNotify = m_notify_mid ;
		dsb[ 1 ].dwOffset = desc.dwBufferBytes - 1 ;
		dsb[ 1 ].hEventNotify = m_notify_end ;
		if ( dsNotify->SetNotificationPositions( 2, dsb ) != S_OK )
		{
			return S_FALSE ;
		}

		m_dwBufferBytes = desc.dwBufferBytes ;
		return S_OK ;
	}
	
	HRESULT ResetPlayLooping()
	{
		if ( this->StopPlay() != S_OK )
		{
			return S_FALSE ;
		}
		
		if ( m_dsound_buffer->Play( 0, 0, DSBPLAY_LOOPING ) != S_OK )
		{
			return S_FALSE ;
		}
		
		return S_OK ;
	}

	HRESULT StopPlay()
	{
		if ( !m_dsound_buffer )
		{
			return S_FALSE ;
		}
		
		m_dsound_buffer->Stop() ;
		
		LPVOID audio_buffer_ptr = nullptr ;
		DWORD audio_buffer_size = 0 ;
		if ( m_dsound_buffer->Lock( 0, m_dwBufferBytes,
			&audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) == S_OK )
		{
			SecureZeroMemory( audio_buffer_ptr, audio_buffer_size ) ;
			m_dsound_buffer->Unlock( audio_buffer_ptr, audio_buffer_size, nullptr, 0 ) ;
		}

		return m_dsound_buffer->SetCurrentPosition( 0 ) ;
	}

	HRESULT LoopBuffer( std::function< void( void*, size_t ) > cb )
	{
		LPVOID audio_buffer_ptr = nullptr ;
		DWORD audio_buffer_size = 0 ;

		HANDLE hEvents[ 2 ] = { 0 } ;
		hEvents[ 0 ] = m_notify_mid ;
		hEvents[ 1 ] = m_notify_end ;
		switch ( MsgWaitForMultipleObjects( 2, hEvents, FALSE, INFINITE, QS_ALLINPUT ) )
		{
		case WAIT_OBJECT_0 :
			m_dsound_buffer->Lock( 0, m_dwBufferBytes / 2,
				&audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) ;
			break ;

		case WAIT_OBJECT_0 + 1 :
			m_dsound_buffer->Lock( m_dwBufferBytes / 2, m_dwBufferBytes / 2,
				&audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) ;
			break ;
		}

		if ( audio_buffer_size )
		{
			cb( audio_buffer_ptr, audio_buffer_size ) ;
			m_dsound_buffer->Unlock( audio_buffer_ptr, audio_buffer_size, nullptr, 0 ) ;
		}
		
		return S_OK ;
	}

private:
	CComQIPtr< IDirectSoundBuffer,
		&IID_IDirectSoundBuffer > m_dsound_buffer ;

	DWORD			m_dwBufferBytes = 0	;
	CAutoHandle		m_notify_mid		;
	CAutoHandle		m_notify_end		;
};

//////////////////////////////////////////////////////////////////////////
