
#ifdef WIN32

#ifndef __sl_http_hpp
#define __sl_http_hpp

//////////////////////////////////////////////////////////////////////////

#include <http.h>
#pragma comment( lib, "httpapi.lib" )

#define INITIALIZE_HTTP_RESPONSE( resp, status, reason )    \
	do														\
	{														\
		RtlZeroMemory( (resp), sizeof(*(resp)) );			\
		(resp)->StatusCode = (status);                      \
		(resp)->pReason = (reason);                         \
		(resp)->ReasonLength = (USHORT) strlen(reason);     \
	} while (FALSE)

#define ADD_KNOWN_HEADER(Response, HeaderId, RawValue)				\
	do																\
	{																\
		(Response).Headers.KnownHeaders[(HeaderId)].pRawValue =		\
		(RawValue);													\
		(Response).Headers.KnownHeaders[(HeaderId)].RawValueLength =\
		(USHORT) strlen(RawValue);									\
	} while(FALSE)

interface IWIN_HTTP_SERVER_EVENT
{
	virtual int OnPostReceiveFile( pair< int, string >& resp, const wchar_t* full_url, const wchar_t* file ) = 0 ;
	virtual int OnGetRequest( pair< int, string >& resp, const wchar_t* full_url, string& str_chunk ) = 0 ;
	virtual int OnServerIdle() = 0 ;
};

//////////////////////////////////////////////////////////////////////////

class CWinHTTPInitV1
{
public:
	CWinHTTPInitV1()
	{
		HTTPAPI_VERSION Version = HTTPAPI_VERSION_1 ;
		HttpInitialize( Version, HTTP_INITIALIZE_SERVER, NULL ) ;
	}

	~CWinHTTPInitV1()
	{
		HttpTerminate( HTTP_INITIALIZE_SERVER, NULL ) ;
	}
};

//////////////////////////////////////////////////////////////////////////

class CWinHTTPServer : public IST
{
public:
	CWinHTTPServer() : m_io_size( 1024 * 1024 )
	{
		m_io_buffer = make_unique< byte[] >( m_io_size ) ;
		if ( m_io_buffer )
		{
			HttpCreateHttpHandle( m_io_http, 0 ) ;
		}

		if ( m_io_http )
		{
			m_iocp = CreateIoCompletionPort( m_io_http, NULL, 0, 0 ) ;
		}

		if ( m_iocp )
		{
			m_http_request = (PHTTP_REQUEST)m_io_buffer.get() ;
		}
		
		m_event_ptr = nullptr ;
	}

public:
	int StartHTTPServer( const wchar_t* URL, IWIN_HTTP_SERVER_EVENT* pEvent )
	{
		if ( HttpAddUrl( m_io_http, URL, NULL ) != NO_ERROR )
		{
			return -1 ;
		}

		if ( m_st_http.StartThread( this ) != 0 )
		{
			return -1 ;
		}
		
		if ( ReceiveHttpRequest() != NO_ERROR )
		{
			return -1 ;
		}
		
		m_event_ptr = pEvent ;
		return 0 ;
	}

private:
	ULONG ReceiveHttpRequest()
	{
		if ( !m_iocp )
		{
			return ERROR_NOT_ENOUGH_MEMORY ;
		}

		SecureZeroMemory( (byte*)m_http_request, m_io_size ) ;
		SecureZeroMemory( &m_overlapped, sizeof( m_overlapped ) ) ;

		HTTP_REQUEST_ID requestId ;
		HTTP_SET_NULL_ID( &requestId ) ;
		
		const ULONG result = HttpReceiveHttpRequest( m_io_http, requestId, 0, m_http_request, m_io_size, NULL, &m_overlapped ) ;
		if ( result == ERROR_IO_PENDING )
		{
			return NO_ERROR ;
		}

		return result ;
	}

	ULONG OnHttpRequest()
	{
		ULONG result = ERROR_CONNECTION_INVALID ;
		
		switch ( m_http_request->Verb )
		{
			case HttpVerbGET :
			{
				result = SendHttpGetResponse() ;
				break ;
			}
			
			case HttpVerbPOST :
			{
				result = SendHttpPostResponse() ;
				break ;
			}
			
			default:
			{
				result = SendHttpResponse( 503, "Not Implemented", nullptr ) ;
				break ;
			}
		}
		
		return result ;
	}
	
	ULONG SendHttpResponse( USHORT StatusCode, PSTR pReason, PSTR pEntityString )
	{
		HTTP_RESPONSE response ;
		INITIALIZE_HTTP_RESPONSE( &response, StatusCode, pReason ) ;
		ADD_KNOWN_HEADER( response, HttpHeaderContentType, "text/html" ) ;
		
		HTTP_DATA_CHUNK dataChunk ;
		if ( pEntityString )
		{
			if ( strlen( pEntityString ) )
			{
				dataChunk.DataChunkType = HttpDataChunkFromMemory ;
				dataChunk.FromMemory.pBuffer = pEntityString ;
				dataChunk.FromMemory.BufferLength = (ULONG)strlen( pEntityString ) ;
				
				response.EntityChunkCount = 1 ;
				response.pEntityChunks = &dataChunk ;
			}
		}
		
		return HttpSendHttpResponse( m_io_http, m_http_request->RequestId, 0, &response, NULL, NULL, NULL, 0, NULL, NULL ) ;
	}

	ULONG SendHttpGetResponse()
	{
		pair< int, string > resp = make_pair( 200, "OK" ) ;

		string str_chunk ;
		
		if ( m_event_ptr != nullptr )
		{
			m_event_ptr->OnGetRequest( resp, m_http_request->CookedUrl.pFullUrl, str_chunk ) ;
		}
		
		return SendHttpResponse( resp.first, (PSTR)resp.second.c_str(), (PSTR)str_chunk.c_str() ) ;
	}

	ULONG SendHttpPostResponse()
	{
		pair< int, string > resp = make_pair( 200, "OK" ) ;

		if ( m_http_request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS )
		{
			wstring file ;
			if ( TransportPostResponse( file ) == NO_ERROR )
			{
				if ( m_event_ptr != nullptr )
				{
					m_event_ptr->OnPostReceiveFile( resp, m_http_request->CookedUrl.pFullUrl, file.c_str() ) ;
				}
			}
		}
		
		return SendHttpResponse( resp.first, (PSTR)resp.second.c_str(), nullptr ) ;
	}
	
	ULONG TransportPostResponse( wstring& file )
	{
		if ( true )
		{
			wchar_t tmp[ MAX_PATH ] = { 0 } ;
			GetTempPath( MAX_PATH, tmp ) ;
			GetTempFileName( tmp, NULL, 0, tmp ) ;
			file = tmp ;
		}
		
		CAutoFile hTempFile = CreateFile( file.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) ;
		if ( !hTempFile )
		{
			return ERROR_NOT_ENOUGH_MEMORY ;
		}

		for ( ULONG TotalBytesRead = 0, BytesRead = 0 ; ; )
		{
			CHAR EntityBuffer[ 1024 ] = { 0 } ;

			BytesRead = 0 ;
			const ULONG result = HttpReceiveRequestEntityBody( m_io_http, m_http_request->RequestId, 0, EntityBuffer, sizeof( EntityBuffer ), &BytesRead, NULL ) ;

			switch ( result )
			{
				case NO_ERROR :
				{
					if ( BytesRead != 0 )
					{
						TotalBytesRead += BytesRead ;
						DWORD TempFileBytesWritten = 0 ;
						WriteFile( hTempFile, EntityBuffer, BytesRead, &TempFileBytesWritten, NULL ) ;
					}

					break ;
				}

				case ERROR_HANDLE_EOF :
				{
					if ( BytesRead != 0 )
					{
						TotalBytesRead += BytesRead ;
						DWORD TempFileBytesWritten = 0 ;
						WriteFile( hTempFile, EntityBuffer, BytesRead, &TempFileBytesWritten, NULL ) ;
					}
					
					return NO_ERROR ;
				}

				default :
				{
					return result ;
				}
			}
		}

		return NO_ERROR ;
	}

private: // interface IST
	virtual int IST_Routine( boost::thread::id tid )
	{
		DWORD NumberOfBytesTransferred = 0 ;
		ULONG_PTR CompletionKey = 0 ;
		LPOVERLAPPED lpOverlapped = nullptr ;
		GetQueuedCompletionStatus( m_iocp, &NumberOfBytesTransferred, &CompletionKey, &lpOverlapped, 100 ) ;
		if ( NumberOfBytesTransferred == 0 && CompletionKey == 0 && lpOverlapped == nullptr )
		{
			if ( m_event_ptr != nullptr )
			{
				m_event_ptr->OnServerIdle() ;
			}

			return 0 ;
		}
		
		OnHttpRequest() ;
		ReceiveHttpRequest() ;
		return 0 ;
	}

private:
	unique_ptr< byte[] >	m_io_buffer		;
	const size_t			m_io_size		;
	OVERLAPPED				m_overlapped	;
	PHTTP_REQUEST			m_http_request	;
	IWIN_HTTP_SERVER_EVENT*	m_event_ptr		;

private:
	CAutoHandle		m_io_http	;
	CAutoHandle		m_iocp		;
	CSmartThread	m_st_http	;
};

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_http_hpp

#endif // #ifdef WIN32
