
#ifdef WIN32

#ifndef __sl_vpp_hpp
#define __sl_vpp_hpp

#ifdef __sl_boost_hpp

class CSmartVPP final
{
private:
	typedef std::function< int( bool, void*, DWORD, ULONG_PTR, void* ) > CBTYPE ;

public:
	CSmartVPP()
	{

	}

	~CSmartVPP()
	{
		this->Uninit() ;
	}

public:
	int Init( CBTYPE cb, DWORD dwWait = INFINITE )
	{
		try
		{
			if ( m_iocp || m_thread )
			{
				throw -1 ;
			}

			m_cb = cb ;
			if ( !m_cb )
			{
				throw -1 ;
			}

			m_iocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 1 ) ;
			if ( !m_iocp )
			{
				throw -1 ;
			}
			
			if ( m_thread.StartThread( std::bind( &CSmartVPP::Routine_Thread, this, true, dwWait ) ) != 0 )
			{
				throw -1 ;
			}
		}

		catch ( ... )
		{
			return -1 ;
		}

		return 0 ;
	}

	void Uninit()
	{
		this->PushBackStatus( 0, 0, nullptr ) ;
		m_thread.ExitThread() ;
		
		this->Cleanup_IOCP() ;
		m_iocp.FreeHandle() ;
	}

public:
	BOOL PushBackStatus( DWORD t1, ULONG_PTR t2, void* t3 )
	{
		if ( m_iocp && m_thread )
		{
			return PostQueuedCompletionStatus( m_iocp,
				t1, t2, (LPOVERLAPPED)t3 ) ;
		}
		
		return FALSE ;
	}

private:
	void Cleanup_IOCP()
	{
		while ( m_cb )
		{
			this->Routine_Thread( false, 0 ) ;
		}
	}

	int Routine_Thread( bool flag, DWORD dwWait )
	{
		DWORD t1 = 0 ;
		ULONG_PTR t2 = 0 ;
		LPOVERLAPPED t3 = nullptr ;

		if ( GetQueuedCompletionStatus( m_iocp,
			&t1,
			&t2,
			&t3,
			dwWait ) )
		{
			return m_cb( flag, this, t1, t2, t3 ) ;
		}
		
		m_cb = nullptr ;
		return 1 ;
	}

private:
	CBTYPE			m_cb		;
	CAutoHandle		m_iocp		;
	CSmartThread	m_thread	;
};

#endif // __sl_boost_hpp

#endif // #ifndef __sl_vpp_hpp

#endif // #ifdef WIN32
