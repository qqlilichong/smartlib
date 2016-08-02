
#ifdef WIN32

#ifndef __sl_xaudio2_hpp
#define __sl_xaudio2_hpp

#include <xaudio2.h>
#pragma comment( lib, "xaudio2" )

//////////////////////////////////////////////////////////////////////////

DECLARE_SLSH( CXA2Master, IXAudio2MasteringVoice*, nullptr, handle ) ;
DECLARE_SLSH( CXA2Source, IXAudio2SourceVoice*, nullptr, handle ) ;

class CXA2Engine
{
public:
	CXA2Engine()
	{
		CoInitializeEx( NULL, COINIT_MULTITHREADED ) ;
		XAudio2Create( &m_engine ) ;
	}

	operator bool ()
	{
		return m_engine != nullptr ;
	}

	operator IXAudio2* ()
	{
		return m_engine ;
	}

	IXAudio2* operator -> ()
	{
		return m_engine ;
	}

private:
	CComQIPtr< IXAudio2 >	m_engine ;
};

//////////////////////////////////////////////////////////////////////////

class CXA2Render
{
public:
	int CreateRender( IXAudio2* pXA2, const WAVEFORMATEX* pWFM )
	{
		if ( pXA2 == nullptr )
		{
			return -1 ;
		}

		if ( pWFM == nullptr )
		{
			return -1 ;
		}

		pXA2->CreateMasteringVoice( m_master ) ;
		if ( !m_master )
		{
			return -1 ;
		}

		pXA2->CreateSourceVoice( m_source, pWFM ) ;
		if ( !m_source )
		{
			return -1 ;
		}

		m_deque.clear() ;
		
		if ( m_source->Start() != S_OK )
		{
			return -1 ;
		}

		return 0 ;
	}

	int KeepSafeDeque()
	{
		if ( !m_source )
		{
			return -1 ;
		}
		
		XAUDIO2_VOICE_STATE state = { 0 } ;
		m_source->GetState( &state ) ;
		if ( state.BuffersQueued > XAUDIO2_MAX_QUEUED_BUFFERS / 2 )
		{
			return -1 ;
		}

		while ( m_deque.size() > state.BuffersQueued )
		{
			m_deque.pop_front() ;
		}

		return 0 ;
	}
	
	int SubmitSourceBuffer( const void* data_ptr, const size_t data_size )
	{
		if ( !m_source )
		{
			return -1 ;
		}
		
		XAUDIO2_BUFFER xa2_buf = { 0 } ;
		xa2_buf.AudioBytes = data_size ;
		auto _buf_ptr = std::make_unique< uint8_t[] >( data_size ) ;
		if ( !_buf_ptr )
		{
			return -1 ;
		}

		xa2_buf.pAudioData = _buf_ptr.get() ;
		memcpy( (void*)xa2_buf.pAudioData, data_ptr, data_size ) ;
		m_deque.emplace_back( xa2_buf, std::move( _buf_ptr ) ) ;
		if ( m_source->SubmitSourceBuffer( &m_deque.back().first ) != S_OK )
		{
			return -1 ;
		}

		return 0 ;
	}

private:
	std::deque< std::pair< XAUDIO2_BUFFER,
		std::unique_ptr< uint8_t[] > > > m_deque ;
	
	CXA2Master	m_master ;
	CXA2Source	m_source ;
};

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_xaudio2_hpp

#endif // #ifdef WIN32
