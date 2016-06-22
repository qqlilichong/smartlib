
#ifndef __sl_string_hpp
#define __sl_string_hpp

#ifdef __sl_boost_hpp

//////////////////////////////////////////////////////////////////////////

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/locale.hpp>

inline wstring SL_StringA2W( const char* str, const char* charset = "GBK" )
{
	return std::move( boost::locale::conv::to_utf< wchar_t >( str, charset ) ) ;
}

inline string SL_StringA2U8( const char* str, const char* charset = "GBK" )
{
	return std::move( boost::locale::conv::to_utf< char >( str, charset ) ) ;
}

inline string SL_StringW2A( const wchar_t* str, const char* charset = "GBK" )
{
	return std::move( boost::locale::conv::from_utf( str, charset ) ) ;
}

inline string SL_StringW2U8( const wchar_t* str )
{
	return std::move( boost::locale::conv::utf_to_utf< char, wchar_t >( str ) ) ;
}

inline string SL_StringU82A( const char* str, const char* charset = "GBK" )
{
	return std::move( boost::locale::conv::from_utf( str, charset ) ) ;
}

inline wstring SL_StringU82W( const char* str )
{
	return std::move( boost::locale::conv::utf_to_utf< wchar_t, char >( str ) ) ;
}

template< class S, class F1, class F2 >
inline void SL_StringReplaceTail( S& str, F1 flag, F2 re )
{
	auto reg = boost::find_last( str, flag ) ;
	if ( reg )
	{
		boost::replace_tail( str, str.begin() - reg.begin(), re ) ;
	}
}

//////////////////////////////////////////////////////////////////////////

#include <boost/filesystem.hpp>
#include <boost/log/attributes.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/info_parser.hpp>

template< class BPT, class SUP >
class CProfileSave : public SUP
{
public:
	bool Load()
	{
		boost::filesystem::path p( boost::log::aux::get_process_name() ) ;
		p = boost::filesystem::system_complete( p ) ;
		p.replace_extension( SUP::Adapter() ) ;
		m_file = std::move( p.generic_string() ) ;
		
		return SUP::LoadAdapter( m_file, m_bpt, m_what ) ;
	}

	bool Load2( const char* add )
	{
		boost::filesystem::path p( boost::log::aux::get_process_name() ) ;
		p = boost::filesystem::system_complete( p ) ;
		p.remove_filename() ;
		p /= add ;
		m_file = std::move( p.generic_string() ) ;

		return SUP::LoadAdapter( m_file, m_bpt, m_what ) ;
	}

	bool Save()
	{
		return SUP::SaveAdapter( m_file, m_bpt, m_what ) ;
	}

	template< class T >
	bool EnumChild( T& t )
	{
		for ( auto& it : m_bpt )
		{
			t.push_back( it.first ) ;
		}

		return !t.empty() ;
	}
	
	const char* LastError() const
	{
		return m_what.c_str() ;
	}

protected:
	string	m_file	;
	string	m_what	;
	BPT		m_bpt	;
};

template< class BPT, class OS, class SUP >
class CProfileSaveStream : public SUP
{
public:
	bool Save()
	{
		return SUP::SaveAdapter( m_stream, m_bpt, m_what ) ;
	}
	
	const char* LastError() const
	{
		return m_what.c_str() ;
	}

	template< class OSSTR >
	void GetString( OSSTR& str )
	{
		str = m_stream.str() ;
	}

protected:
	OS		m_stream	;
	string	m_what		;
	BPT		m_bpt		;
};

class CProfileSave_XML
{
protected:
	const char* Adapter() const
	{
		return "xml" ;
	}

	template< class T1, class T2, class T3 >
	bool LoadAdapter( T1& out, T2& bpt, T3& what )
	{
		try
		{
			boost::property_tree::read_xml( out, bpt ) ;
		}

		catch ( std::exception& exp )
		{
			what = exp.what() ;
			return false ;
		}

		return true ;
	}

	template< class T1, class T2, class T3 >
	bool SaveAdapter( T1& out, T2& bpt, T3& what )
	{
		try
		{
			boost::property_tree::write_xml( out, bpt ) ;
		}

		catch ( std::exception& exp )
		{
			what = exp.what() ;
			return false ;
		}

		return true ;
	}
};

class CProfileSave_INI
{
protected:
	const char* Adapter() const
	{
		return "ini" ;
	}

	template< class T1, class T2, class T3 >
	bool LoadAdapter( T1& out, T2& bpt, T3& what )
	{
		try
		{
			boost::property_tree::read_ini( out, bpt ) ;
		}

		catch ( std::exception& exp )
		{
			what = exp.what() ;
			return false ;
		}

		return true ;
	}

	template< class T1, class T2, class T3 >
	bool SaveAdapter( T1& out, T2& bpt, T3& what )
	{
		try
		{
			boost::property_tree::write_ini( out, bpt ) ;
		}

		catch ( std::exception& exp )
		{
			what = exp.what() ;
			return false ;
		}

		return true ;
	}
};

template< class SAVE >
class CProfile : public SAVE
{
public:
	template< class K, class V >
	V Get( const K& key, const V& val )
	{
		try
		{
			return std::move( SAVE::m_bpt.get( key, val ) ) ;
		}

		catch ( std::exception& exp )
		{
			SAVE::m_what = exp.what() ;
		}

		return val ;
	}
	
	template< class K, class V >
	V Get2( const K& key, const V& val )
	{
		V ret = val ;

		try
		{
			ret = std::move( SAVE::m_bpt.get( key, val ) ) ;
		}

		catch ( std::exception& exp )
		{
			SAVE::m_what = exp.what() ;
		}

		try
		{
			this->Put( key, ret ) ;
			SAVE::Save() ;
		}

		catch ( std::exception& exp )
		{
			SAVE::m_what = exp.what() ;
		}

		return std::move( ret ) ;
	}
	
	template< class K, class V >
	bool Put( const K& key, const V& val )
	{
		try
		{
			SAVE::m_bpt.put( key, val ) ;
		}

		catch ( std::exception& exp )
		{
			SAVE::m_what = exp.what() ;
			return false ;
		}

		return true ;
	}
};

//////////////////////////////////////////////////////////////////////////

#endif // #ifdef __sl_boost_hpp

#endif // #ifndef __sl_string_hpp
