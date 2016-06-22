
#ifndef __sl_live555_hpp
#define __sl_live555_hpp

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>

//////////////////////////////////////////////////////////////////////////

inline void CSmartHandleDeleter_TaskScheduler( TaskScheduler*& handle )
{
	delete handle ;
}

typedef CSmartHandle< TaskScheduler*, nullptr, CSmartHandleDeleter_TaskScheduler > CLive555TaskScheduler ;

inline void CSmartHandleDeleter_UsageEnvironment( UsageEnvironment*& handle )
{
	handle->reclaim() ;
}

typedef CSmartHandle< UsageEnvironment*, nullptr, CSmartHandleDeleter_UsageEnvironment > CLive555UsageEnvironment ;

inline void CSmartHandleDeleter_MediaSession( MediaSession*& handle )
{
	Medium::close( handle ) ;
}

typedef CSmartHandle< MediaSession*, nullptr, CSmartHandleDeleter_MediaSession > CLive555MediaSession ;

//////////////////////////////////////////////////////////////////////////

class CSimSink : public MediaSink
{
public:
	typedef function< void(MediaSubsession*,unsigned,unsigned,struct timeval,unsigned) > fun_type ;

public:
	CSimSink( fun_type cb, UsageEnvironment& env, size_t bufsize ) :
		MediaSink( env ), m_cb( cb ), m_bufsize( bufsize )
	{
		m_buffer = make_unique< byte[] >( m_bufsize ) ;
	}

	virtual ~CSimSink()
	{
		m_buffer.reset() ;
	}

private:
	virtual Boolean continuePlaying()
	{
		if ( !fSource )
		{
			return false ;
		}

		if ( !m_buffer )
		{
			return false ;
		}
		
		fSource->getNextFrame( m_buffer.get(), m_bufsize,
			
		[]( void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds )
		{
			((CSimSink*)clientData)->afterGettingFrame( frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds ) ;
		}
		
		,this, onSourceClosure, this ) ;
		
		return true ;
	}

	void afterGettingFrame( unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds )
	{
		m_cb( (MediaSubsession*)fAfterClientData, frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds ) ;
		this->continuePlaying() ;
	}

private:
	fun_type				m_cb		;
	const size_t			m_bufsize	;
	unique_ptr< byte[] >	m_buffer	;
};

//////////////////////////////////////////////////////////////////////////

class CSimRTSPClient : public RTSPClient
{
public:
	CSimRTSPClient( UsageEnvironment& env, char const* rtspURL ) :
		RTSPClient( env, rtspURL, 0, "SimRTSPClient", 0, -1 )
	{
		m_subms = nullptr ;
	}

	~CSimRTSPClient()
	{
		this->ShutDown() ;
	}

public:
	int AddSink( CSimSink::fun_type cb, const wchar_t* filter, const bool tcp = true, const size_t bufsize = 100000 )
	{
		auto sink_ptr = std::make_shared< CSimSink >( cb, this->envir(), bufsize ) ;
		if ( !sink_ptr )
		{
			return -1 ;
		}
		
		m_media_sink.emplace_back( filter, tcp, sink_ptr ) ;
		return 0 ;
	}

	void SendDescribe( const char* user, const char* pwd )
	{
		Authenticator auth( user, pwd ) ;
		this->sendDescribeCommand(
		
		[]( RTSPClient* obj, int resultCode, char* resultString )
		{
			string str ;
			if ( resultString )
			{
				str = resultString ;
				delete []resultString ;
				resultString = nullptr ;
			}

			( (CSimRTSPClient*)obj )->ContinueDescribe( resultCode, str ) ;
		}
		
		,&auth ) ;
		
		this->envir().taskScheduler().doEventLoop( &m_loop ) ;
	}

	void StopLoop()
	{
		m_loop = 1 ;
	}
	
	void ShutDown()
	{
		if ( m_media )
		{
			this->sendTeardownCommand( *m_media.get(), nullptr ) ;
		}

		m_subms = nullptr ;
		m_media_sink.clear() ;
		m_media_iter.reset() ;
		m_media.FreeHandle() ;
	}

private:
	int ContinueDescribe( const int resultCode, const string& info )
	{
		try
		{
			if ( SL_ASSERT( resultCode == 0, resultCode ) )
			{
				throw -1 ;
			}
		
			m_media = MediaSession::createNew( this->envir(), info.c_str() ) ;
			if ( SL_ASSERT( m_media, L"ContinueDescribe" ) )
			{
				throw -1 ;
			}
		
			if ( SL_ASSERT( m_media->hasSubsessions(), L"ContinueDescribe" ) )
			{
				throw -1 ;
			}
		
			m_media_iter = std::make_shared< MediaSubsessionIterator >( *m_media.get() ) ;
			if ( SL_ASSERT( m_media_iter, L"ContinueDescribe" ) )
			{
				throw -1 ;
			}
		}

		catch ( ... )
		{
			return -1 ;
		}

		m_subms = nullptr ;
		return this->SetupSubsession() ;
	}

	int ContinueSetup( const int resultCode, const string& info )
	{
		try
		{
			auto codec_name = SL_StringA2W( m_subms->codecName() ) ;

			if ( SL_ASSERT( resultCode == 0, codec_name ) )
			{
				throw -1 ;
			}
			
			auto ptr = (MediaSink*)m_subms->miscPtr ;
			if ( SL_ASSERT( ptr->startPlaying( *m_subms->readSource(), nullptr, m_subms ), codec_name ) )
			{
				throw -1 ;
			}
		}

		catch ( ... )
		{
			this->StopLoop() ;
			return -1 ;
		}
		
		return this->SetupSubsession() ;
	}

	int StartPlay()
	{
		this->sendPlayCommand( *m_media.get(), 
			
		[]( RTSPClient* obj, int resultCode, char* resultString )
		{
			string str ;
			if ( resultString )
			{
				str = resultString ;
				delete []resultString ;
				resultString = nullptr ;
			}

			if ( SL_ASSERT( resultCode == 0, L"StartPlayError" ) )
			{
				( ( CSimRTSPClient*)obj )->StopLoop() ;
			}
		}
		
		) ;

		return 0 ;
	}

	int SetupSubsession()
	{
		m_subms = m_media_iter->next() ;
		if ( m_subms == nullptr )
		{
			return this->StartPlay() ;
		}
		
		auto codec_name = SL_StringA2W( m_subms->codecName() ) ;
		
		auto subpos = std::find_if(
			
			m_media_sink.cbegin(),
			m_media_sink.cend(),
			
			[ &codec_name ]( const tsink& tt )
			{
				return std::get< 0 >( tt ) == codec_name ;
			}
		) ;
		
		if ( SL_ASSERT( subpos != m_media_sink.cend(), codec_name ) )
		{
			return this->SetupSubsession() ;
		}

		if ( SL_ASSERT( m_subms->initiate(), codec_name ) )
		{
			return this->SetupSubsession() ;
		}
		
		m_subms->miscPtr = std::get< 2 >( *subpos ).get() ;
		
		this->sendSetupCommand( *m_subms,
			
		[]( RTSPClient* obj, int resultCode, char* resultString )
		{
			auto clt = (CSimRTSPClient*)obj ;
			
			string str ;
			if ( resultString )
			{
				str = resultString ;
				delete []resultString ;
				resultString = nullptr ;
			}
			
			clt->ContinueSetup( resultCode, str ) ;
		}
		
		, false, std::get< 1 >( *subpos ) ) ;
		
		return 0 ;
	}

private:
	typedef tuple< wstring, bool, shared_ptr< MediaSink > > tsink ;
	
	char									m_loop = 0		;
	CLive555MediaSession					m_media			;
	shared_ptr< MediaSubsessionIterator >	m_media_iter	;
	vector< tsink >							m_media_sink	;
	MediaSubsession*						m_subms			;
};

//////////////////////////////////////////////////////////////////////////

/*
inline void TestLive555()
{
	CLive555TaskScheduler scheduler( BasicTaskScheduler::createNew() ) ;
	CLive555UsageEnvironment env( BasicUsageEnvironment::createNew( *scheduler.get() ) ) ;

	auto rtsp_ptr = 
		std::make_shared< CSimRTSPClient >( *env.get(),
		"rtsp://192.168.0.223:554/PSIA/streaming/channels/201" ) ;
	
	if ( !SL_ASSERT( rtsp_ptr, L"live555-error" ) )
	{
		auto& video_cb = 
		[ & ] ( MediaSubsession* subms, 
		unsigned frameSize, 
		unsigned numTruncatedBytes, 
		struct timeval presentationTime, 
		unsigned durationInMicroseconds )
		{

		} ;

		auto& audio_cb = 
		[ & ] ( MediaSubsession* subms, 
		unsigned frameSize, 
		unsigned numTruncatedBytes, 
		struct timeval presentationTime, 
		unsigned durationInMicroseconds )
		{

		} ;
		
		rtsp_ptr->AddSink( video_cb, L"MPV" ) ;
		rtsp_ptr->AddSink( audio_cb, L"PCMU" ) ;
		rtsp_ptr->SendDescribe( "admin", "pass" ) ;
	}
}*/

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_live555_hpp
