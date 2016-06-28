
#ifndef __sl_time_hpp
#define __sl_time_hpp

#include <chrono>

//////////////////////////////////////////////////////////////////////////

class CSteadyClock final
{
private:
	typedef std::chrono::steady_clock clock_type ;
	clock_type::time_point m_clock = clock_type::now() ;

public:
	CSteadyClock	() = default ;
	~CSteadyClock	() = default ;

public:
	operator bool ()
	{
		decltype( m_clock ) clear_clock ;
		return m_clock != clear_clock ;
	}

	void Clear()
	{
		decltype( m_clock ) clear_clock ;
		m_clock = clear_clock ;
	}

	void Reset()
	{
		m_clock = clock_type::now() ;
	}
	
	clock_type::duration Elapsed()
	{
		return clock_type::now() - m_clock ;
	}
};

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_time_hpp
