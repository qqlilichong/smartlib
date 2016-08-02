
#ifndef __sl_memory_hpp
#define __sl_memory_hpp

//////////////////////////////////////////////////////////////////////////

template< class handle_type, handle_type invalid_val, void(&free_sink)( handle_type& ) >
class CSmartHandle final
{
public:
	CSmartHandle( const CSmartHandle& obj ) = delete ;
	CSmartHandle& operator = ( const CSmartHandle& obj ) = delete ;

	CSmartHandle& operator = ( CSmartHandle&& obj )
	{
		return *this = obj.DetachHandle() ;
	}
	
	CSmartHandle( CSmartHandle&& tmp )
	{
		*this = std::move( tmp ) ;
	}
	
	CSmartHandle( handle_type handle )
	{
		m_handle = handle ;
	}
	
	CSmartHandle() : CSmartHandle( invalid_val )
	{

	}
	
	~CSmartHandle()
	{
		this->FreeHandle() ;
	}

public:
	void FreeHandle()
	{
		if ( m_handle != invalid_val )
		{
			free_sink( m_handle ) ;
			m_handle = invalid_val ;
		}
	}
	
	handle_type DetachHandle()
	{
		auto tmp = std::move( m_handle ) ;
		m_handle = invalid_val ;
		return tmp ;
	}
	
	handle_type get()
	{
		return m_handle ;
	}
	
	CSmartHandle& operator = ( handle_type handle )
	{
		this->FreeHandle() ;
		m_handle = handle ;
		return *this ;
	}

	operator bool ()
	{
		return m_handle != invalid_val ;
	}
	
	operator handle_type ()
	{
		return m_handle ;
	}
	
	operator handle_type* ()
	{
		return &m_handle ;
	}
	
	handle_type	operator -> ()
	{
		return m_handle ;
	}

private:
	handle_type	m_handle ;
};

#define	DECLARE_SLSH( name, val, nul, fun ) \
		inline void CSmartHandleDeleter_##name( val& handle ) { fun ; } \
		using name = CSmartHandle< val, nul, CSmartHandleDeleter_##name > ;

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_memory_hpp
