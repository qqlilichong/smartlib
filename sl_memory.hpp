
#ifndef __sl_memory_hpp
#define __sl_memory_hpp

#include <stdint.h>

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

DECLARE_SLSH( CByteBuffer, uint8_t*, nullptr, delete []handle ) ;

class CByteStream
{
public:
	int CreateByteStream( size_t buffer_max )
	{
		m_buffer_stream = new uint8_t[ buffer_max ] ;
		if ( !m_buffer_stream )
		{
			return -1 ;
		}
		
		m_buffer_max = buffer_max ;
		m_buffer_offset = 0 ;
		return 0 ;
	}

	int FreeByteStream()
	{
		m_buffer_max = 0 ;
		m_buffer_offset = 0 ;
		m_buffer_stream.FreeHandle() ;
		return 0 ;
	}

	int Fill( void* src_ptr, size_t src_size )
	{
		if ( !m_buffer_stream )
		{
			return -1 ;
		}
		
		if ( ( m_buffer_offset + src_size ) > m_buffer_max )
		{
			return -1 ;
		}
		
		uint8_t* dst_ptr = m_buffer_stream ;
		memcpy( dst_ptr + m_buffer_offset, src_ptr, src_size ) ;
		m_buffer_offset += src_size ;
		return 0 ;
	}
	
	int Pick( void* dst_ptr, size_t pick_size )
	{
		if ( !m_buffer_stream )
		{
			return -1 ;
		}
		
		if ( m_buffer_offset < pick_size )
		{
			return -1 ;
		}
		
		uint8_t* src_ptr = m_buffer_stream ;
		memcpy( dst_ptr, src_ptr, pick_size ) ;
		
		const size_t new_offset = m_buffer_offset - pick_size ;
		memcpy( src_ptr, src_ptr + pick_size, new_offset ) ;
		m_buffer_offset = new_offset ;
		
		return 0 ;
	}

	operator bool ()
	{
		return m_buffer_stream ;
	}

private:
	size_t		m_buffer_max	= 0 ;
	size_t		m_buffer_offset	= 0	;
	CByteBuffer	m_buffer_stream		;
};

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_memory_hpp
