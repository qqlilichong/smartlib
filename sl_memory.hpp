
#ifndef __sl_memory_hpp
#define __sl_memory_hpp

//////////////////////////////////////////////////////////////////////////

template< class handle_type, handle_type invalid_val, void (*free_sink)(handle_type&) >
class CSmartHandle
{
public:
	CSmartHandle()
	{
		m_handle = invalid_val ;
	}

	CSmartHandle( handle_type handle )
	{
		m_handle = handle ;
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
		handle_type ret = m_handle ;
		m_handle = invalid_val ;
		return ret ;
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

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_memory_hpp
