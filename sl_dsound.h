
#pragma once

#include <mmreg.h>
#include <dsound.h>
#pragma comment( lib, "dsound.lib" )

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
	CComQIPtr< IDirectSound, &IID_IDirectSound > m_dsound ;
};

//////////////////////////////////////////////////////////////////////////

class CDSoundBuffer
{
public:
	typedef std::function< int( void*, size_t ) > CBTYPE ;

	CDSoundBuffer()
	{
		m_wfm = { 0 } ;
		m_desc = { 0 } ;
	}

	~CDSoundBuffer()
	{
		this->FreeDSoundBuffer() ;
	}

	int CreateDSoundBuffer( IDirectSound* pDS, DWORD nSamplesPerSec, WORD nChannels, WORD wBitsPerSample )
	{
		if ( m_dsbuf )
		{
			return -1 ;
		}

		m_play_pos = 0 ;
		
		m_wfm = { 0 } ;
		m_wfm.wFormatTag = WAVE_FORMAT_PCM ;
		m_wfm.nChannels = nChannels ;
		m_wfm.nSamplesPerSec = nSamplesPerSec ;
		m_wfm.wBitsPerSample = wBitsPerSample ;
		m_wfm.nBlockAlign = m_wfm.wBitsPerSample / 8 * m_wfm.nChannels ;
		m_wfm.nAvgBytesPerSec = m_wfm.nSamplesPerSec * m_wfm.nBlockAlign ;
		
		m_desc = { 0 } ;
		m_desc.dwSize = sizeof( m_desc ) ;
		m_desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPAN ;
		m_desc.dwBufferBytes = m_wfm.nAvgBytesPerSec / 2 ;
		m_desc.lpwfxFormat = &m_wfm ;

		if ( pDS->CreateSoundBuffer( &m_desc, &m_dsbuf, NULL ) != S_OK )
		{
			return -1 ;
		}

		return 0 ;
	}

	int FreeDSoundBuffer()
	{
		this->StopPlay() ;

		m_wfm = { 0 } ;
		m_desc = { 0 } ;
		m_dsbuf.Release() ;
		return 0 ;
	}
	
	int ResetPlayLooping()
	{
		if ( this->StopPlay() != 0 )
		{
			return -1 ;
		}
		
		if ( m_dsbuf->Play( 0, 0, DSBPLAY_LOOPING ) != S_OK )
		{
			return -1 ;
		}
		
		return 0 ;
	}

	int StopPlay()
	{
		if ( !m_dsbuf )
		{
			return -1 ;
		}

		m_dsbuf->Stop() ;
		m_play_pos = 0 ;

		if ( true )
		{
			LPVOID audio_buffer_ptr = nullptr ;
			DWORD audio_buffer_size = 0 ;
			if ( m_dsbuf->Lock( 0, m_desc.dwBufferBytes,
				&audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) != S_OK )
			{
				return -1 ;
			}
			
			SecureZeroMemory( audio_buffer_ptr, audio_buffer_size ) ;
			m_dsbuf->Unlock( audio_buffer_ptr, audio_buffer_size, nullptr, 0 ) ;
		}
		
		if ( m_dsbuf->SetCurrentPosition( 0 ) != S_OK )
		{
			return -1 ;
		}

		return 0 ;
	}
	
	int LoopDSoundBuffer( CBTYPE cb )
	{
		if ( !m_dsbuf || !cb )
		{
			return -1 ;
		}
		
		const DWORD mid_pos = m_desc.dwBufferBytes / 2 ;
		
		DWORD play_pos = 0 ;
		if ( m_dsbuf->GetCurrentPosition( &play_pos, nullptr ) != S_OK )
		{
			return -1 ;
		}
		
		LPVOID audio_buffer_ptr = nullptr ;
		DWORD audio_buffer_size = 0 ;
		
		if ( ( play_pos >= mid_pos ) && ( m_play_pos < mid_pos ) )
		{
			m_dsbuf->Lock( 0, mid_pos, &audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) ;
		}
		
		else if ( play_pos < m_play_pos )
		{
			m_dsbuf->Lock( mid_pos, mid_pos, &audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) ;
		}
		
		m_play_pos = play_pos ;
		
		if ( audio_buffer_size )
		{
			cb( audio_buffer_ptr, audio_buffer_size ) ;
			m_dsbuf->Unlock( audio_buffer_ptr, audio_buffer_size, nullptr, 0 ) ;
		}
		
		return 0 ;
	}

	operator bool ()
	{
		return m_dsbuf != nullptr ;
	}

private:
	CComQIPtr< IDirectSoundBuffer,
		&IID_IDirectSoundBuffer >	m_dsbuf			;
	DWORD							m_play_pos = 0	;
	WAVEFORMATEX					m_wfm			;
	DSBUFFERDESC					m_desc			;
};

//////////////////////////////////////////////////////////////////////////
