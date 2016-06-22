
#ifndef __sl_debug_hpp
#define __sl_debug_hpp

//////////////////////////////////////////////////////////////////////////

#ifdef __sl_boost_hpp

#include <boost/assert.hpp>
#define SL_ASSERT( expr, msg ) ( BOOST_LIKELY( !!(expr) ) ? (false) : (__assertion_failed_msg( msg, #expr, BOOST_CURRENT_FUNCTION, __FILE__, __LINE__ ),true) )
template< class MES >
inline void __assertion_failed_msg(
	MES message,
	char const* expression,
	char const* func,
	char const* file,
	long line )
{
	auto tt = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() ) ;
	
	tm tmp = { 0 } ;
	auto ct = boost::date_time::c_time::localtime( &tt, &tmp ) ;
	if ( !ct )
	{
		return ;
	}
	
	const auto& log = ( boost::wformat(
		L"********************************\n"
		L"%04d-%02d-%02d %02d:%02d:%02d\n"
		L"Expression: %s\n"
		L"Message: %s\n"
		L"Function: %s\n"
		L"File: %s\n"
		L"Line: %ld\n"
		L"--------------------------------\n" )
		% ( ct->tm_year + 1900 ) % ( ct->tm_mon + 1 ) % ct->tm_mday % ct->tm_hour % ct->tm_min % ct->tm_sec
		% expression % message % func % file % line ).str() ;
	
	wcout << log << flush ;

	#ifdef _WIN32
		OutputDebugStringW( log.c_str() ) ;
	#endif
}

#endif // #ifdef __sl_boost_hpp

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_debug_hpp
