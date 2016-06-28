
#ifndef __sl_thread_hpp
#define __sl_thread_hpp

//////////////////////////////////////////////////////////////////////////

#ifdef __sl_boost_hpp

#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>
#include <boost/thread/lock_factories.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/lockable_concepts.hpp>
#include <boost/thread/thread_guard.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/shared_lock_guard.hpp>

class CSmartThread final
{
private:
	typedef std::function< int( boost::thread::id ) > CBTYPE ;

public:
	CSmartThread()
	{

	}

	~CSmartThread()
	{
		this->ExitThread() ;
	}

public:
	int StartThread( CBTYPE cb )
	{
		if ( m_pth )
		{
			this->ExitThread() ;
		}

		if ( cb )
		{
			m_pth = make_shared< boost::thread >(
				[ = ]()
				{
					try
					{
						while ( cb( boost::this_thread::get_id() ) == 0 )
						{
							boost::this_thread::interruption_point() ;
						}
					}
					
					catch ( const boost::thread_interrupted& )
					{

					}
				}
			) ;
		}

		return 0 ;
	}

	void ExitThread()
	{
		if ( m_pth )
		{
			m_pth->interrupt() ;
			m_pth->join() ;
			m_pth.reset() ;
		}
	}
	
	operator bool ()
	{
		return (bool)m_pth ;
	}

	bool operator == ( boost::thread::id tid )
	{
		if ( m_pth && m_pth->get_id() == tid )
		{
			return true ;
		}

		return false ;
	}

private:
	shared_ptr< boost::thread >	m_pth ;
};

#endif // #ifdef __sl_boost_hpp

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_thread_hpp
