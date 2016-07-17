
#ifndef __sl_ffmpeg_hpp
#define __sl_ffmpeg_hpp

//////////////////////////////////////////////////////////////////////////

extern "C"
{
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
}

//////////////////////////////////////////////////////////////////////////

DECLARE_SLSH( CFFmpegAVFormatContext,	AVFormatContext*,	nullptr, avformat_close_input( &handle )	) ;
DECLARE_SLSH( CFFmpegAVCodecContext,	AVCodecContext*,	nullptr, avcodec_close( handle )			) ;
DECLARE_SLSH( CFFmpegAVFrame,			AVFrame*,			nullptr, av_frame_free( &handle )			) ;
DECLARE_SLSH( CFFmpegAVFilterInOut,		AVFilterInOut*,		nullptr, avfilter_inout_free( &handle )		) ;
DECLARE_SLSH( CFFmpegAVFilterGraph,		AVFilterGraph*,		nullptr, avfilter_graph_free( &handle )		) ;
DECLARE_SLSH( CFFmpegRefAVFrame,		AVFrame*,			nullptr, av_frame_unref( handle )			) ;

//////////////////////////////////////////////////////////////////////////

class CFFmpegVideoDecoder
{
private:
	typedef std::function< bool( AVRational, AVFrame* ) > CBTYPE ;

public:
	bool Open( AVFormatContext* input_file, CBTYPE cb )
	{
		const auto idx = av_find_best_stream( input_file, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0 ) ;
		if ( idx < 0 )
		{
			return false ;
		}
		
		m_timebase = input_file->streams[ idx ]->time_base ;
		
		const auto codec_ctx = input_file->streams[ idx ]->codec ;
		if ( codec_ctx == nullptr )
		{
			return false ;
		}
		
		const auto codec = avcodec_find_decoder( codec_ctx->codec_id ) ;
		if ( codec == nullptr )
		{
			return false ;
		}
		
		AVDictionary* opts = nullptr ;
		av_dict_set( &opts, "refcounted_frames", "0", 0 ) ;
		if ( avcodec_open2( codec_ctx, codec, &opts ) < 0 )
		{
			return false ;
		}

		m_frame = av_frame_alloc() ;
		if ( !m_frame )
		{
			return false ;
		}

		m_cb = cb ;
		if ( !m_cb )
		{
			return false ;
		}
		
		codec_ctx->opaque = (void*)idx ;
		return m_decoder = codec_ctx ;
	}

public:
	const int GetStreamIdx()
	{
		if ( m_decoder )
		{
			return (int)m_decoder->opaque ;
		}

		return -1 ;
	}

	operator bool ()
	{
		return m_decoder ;
	}
	
	bool operator () ()
	{
		return m_cb( m_timebase, m_frame ) ;
	}

	operator AVCodecContext* ()
	{
		return m_decoder ;
	}

	operator AVFrame* ()
	{
		return m_frame ;
	}

private:
	CFFmpegAVCodecContext	m_decoder	;
	CFFmpegAVFrame			m_frame		;
	AVRational				m_timebase	;
	CBTYPE					m_cb		;
};

class CFFmpegAudioDecoder
{
private:
	typedef std::function< bool( AVRational, AVFrame* ) > CBTYPE ;

public:
	bool Open( AVFormatContext* input_file, CBTYPE cb )
	{
		const auto idx = av_find_best_stream( input_file, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0 ) ;
		if ( idx < 0 )
		{
			return false ;
		}
		
		m_timebase = input_file->streams[ idx ]->time_base ;
		
		const auto codec_ctx = input_file->streams[ idx ]->codec ;
		if ( codec_ctx == nullptr )
		{
			return false ;
		}
		
		const auto codec = avcodec_find_decoder( codec_ctx->codec_id ) ;
		if ( codec == nullptr )
		{
			return false ;
		}
		
		AVDictionary* opts = nullptr ;
		av_dict_set( &opts, "refcounted_frames", "0", 0 ) ;
		if ( avcodec_open2( codec_ctx, codec, &opts ) < 0 )
		{
			return false ;
		}

		m_frame = av_frame_alloc() ;
		if ( !m_frame )
		{
			return false ;
		}

		m_cb = cb ;
		if ( !m_cb )
		{
			return false ;
		}
		
		codec_ctx->opaque = (void*)idx ;
		return m_decoder = codec_ctx ;
	}

public:
	const int GetStreamIdx()
	{
		if ( m_decoder )
		{
			return (int)m_decoder->opaque ;
		}

		return -1 ;
	}

	operator bool ()
	{
		return m_decoder ;
	}
	
	bool operator () ()
	{
		return m_cb( m_timebase, m_frame ) ;
	}

	operator AVCodecContext* ()
	{
		return m_decoder ;
	}

	operator AVFrame* ()
	{
		return m_frame ;
	}

private:
	CFFmpegAVCodecContext	m_decoder	;
	CFFmpegAVFrame			m_frame		;
	AVRational				m_timebase	;
	CBTYPE					m_cb		;
};

//////////////////////////////////////////////////////////////////////////

class CFFmpegVideoFilterGraph
{
public:
	bool FilterComplex( const char* descr, AVRational timebase, AVFrame* pFrame, AVPixelFormat pix_fmt_out )
	{
		try
		{
			m_graph = avfilter_graph_alloc() ;
			if ( !m_graph )
			{
				throw -1 ;
			}

			auto src_args = ( 
				boost::format( 
				"video_size=%dx%d"
				":pix_fmt=%d"
				":time_base=%d/%d"
				":pixel_aspect=%d/%d" )
				% pFrame->width
				% pFrame->height
				% pFrame->format
				% timebase.num
				% timebase.den
				% pFrame->sample_aspect_ratio.num
				% pFrame->sample_aspect_ratio.den
				).str() ;

			if ( avfilter_graph_create_filter( &m_filter_src,
				avfilter_get_by_name( "buffer" ),
				nullptr,
				src_args.c_str(),
				nullptr,
				m_graph ) < 0 )
			{
				throw -1 ;
			}
			
			if ( avfilter_graph_create_filter( &m_filter_sink,
				avfilter_get_by_name( "buffersink" ),
				nullptr,
				nullptr,
				nullptr,
				m_graph ) < 0 )
			{
				throw -1 ;
			}
			
			enum AVPixelFormat pix_fmts[] = { pix_fmt_out, AV_PIX_FMT_NONE } ;
			if ( av_opt_set_int_list( m_filter_sink,
				"pix_fmts",
				pix_fmts,
				AV_PIX_FMT_NONE,
				AV_OPT_SEARCH_CHILDREN ) < 0 )
			{
				throw -1 ;
			}

			CFFmpegAVFilterInOut outputs = avfilter_inout_alloc() ;
			if ( !outputs )
			{
				throw -1 ;
			}

			CFFmpegAVFilterInOut inputs = avfilter_inout_alloc() ;
			if ( !inputs )
			{
				throw -1 ;
			}

			outputs->name = av_strdup( "in" ) ;
			outputs->filter_ctx = m_filter_src ;
			outputs->pad_idx = 0 ;
			outputs->next = nullptr ;

			inputs->name = av_strdup( "out" ) ;
			inputs->filter_ctx = m_filter_sink ;
			inputs->pad_idx = 0 ;
			inputs->next = nullptr ;

			if ( avfilter_graph_parse_ptr( m_graph, descr, inputs, outputs, nullptr ) < 0 )
			{
				throw -1 ;
			}

			if ( avfilter_graph_config( m_graph, nullptr ) < 0 )
			{
				throw -1 ;
			}

			m_frame_out = av_frame_alloc() ;
			if ( !m_frame_out )
			{
				throw -1 ;
			}
		}

		catch ( ... )
		{
			m_graph.FreeHandle() ;
			return false ;
		}

		return true ;
	}

	CFFmpegRefAVFrame Graph( AVFrame* pFrameIn )
	{
		if ( av_buffersrc_add_frame( m_filter_src, pFrameIn ) < 0 )
		{
			return CFFmpegRefAVFrame( nullptr ) ;
		}

		if ( av_buffersink_get_frame( m_filter_sink, m_frame_out ) < 0 )
		{
			return CFFmpegRefAVFrame( nullptr ) ;
		}

		return CFFmpegRefAVFrame( m_frame_out ) ;
	}
	
	bool GetTimeBase( AVRational& time_base )
	{
		if ( m_filter_sink && m_filter_sink->inputs[ 0 ] )
		{
			time_base = m_filter_sink->inputs[ 0 ]->time_base ;
			return true ;
		}
		
		return false ;
	}

	operator bool ()
	{
		return m_graph ;
	}

private:
	CFFmpegAVFilterGraph	m_graph					;
	CFFmpegAVFrame			m_frame_out				;
	AVFilterContext*		m_filter_src = nullptr	;
	AVFilterContext*		m_filter_sink = nullptr	;
};

//////////////////////////////////////////////////////////////////////////

inline int sl_ffmpeg_decode_packet( bool& decoding_continue, AVPacket& pkt, int& got_frame,
	CFFmpegVideoDecoder& decoder_video, CFFmpegAudioDecoder& decoder_audio )
{
	const auto idx_video = decoder_video.GetStreamIdx() ;
	const auto idx_audio = decoder_audio.GetStreamIdx() ;
	auto decoded = pkt.size ;
	got_frame = 0 ;
	
	if ( pkt.stream_index == idx_video )
	{
		const auto ret = avcodec_decode_video2( decoder_video, decoder_video, &got_frame, &pkt ) ;
		if ( ret < 0 )
		{
			return ret ;
		}

		if ( got_frame )
		{
			decoding_continue = decoder_video() ;
		}
	}

	else if ( pkt.stream_index == idx_audio )
	{
		const auto ret = avcodec_decode_audio4( decoder_audio, decoder_audio, &got_frame, &pkt ) ;
		if ( ret < 0 )
		{
			return ret ;
		}
		
		decoded = FFMIN( ret, pkt.size ) ;
		
		if ( got_frame )
		{
			decoding_continue = decoder_audio() ;
		}
	}
	
	return decoded ;
}

inline void sl_ffmpeg_decoding( AVFormatContext* input_file,
	CFFmpegVideoDecoder& decoder_video, CFFmpegAudioDecoder& decoder_audio )
{
	bool decoding_continue = true ;
	int got_frame = 0 ;
	
	AVPacket pkt ;
	av_init_packet( &pkt ) ;
	pkt.data = nullptr ;
	pkt.size = 0 ;
	
	while ( ( av_read_frame( input_file, &pkt ) >= 0 ) && decoding_continue )
	{
		AVPacket ori_pkt = pkt ;

		do 
		{
			const auto ret = sl_ffmpeg_decode_packet( decoding_continue, pkt, got_frame, decoder_video, decoder_audio ) ;
			if ( ret < 0 )
			{
				break ;
			}

			pkt.data += ret ;
			pkt.size -= ret ;

		} while ( pkt.size > 0 && decoding_continue ) ;

		av_packet_unref( &ori_pkt ) ;
	}
}

inline void sl_ffmpeg_decoding_flush( AVFormatContext* input_file,
	CFFmpegVideoDecoder& decoder_video, CFFmpegAudioDecoder& decoder_audio )
{
	bool decoding_continue = true ;
	int got_frame = 0 ;
	
	AVPacket pkt ;
	av_init_packet( &pkt ) ;
	pkt.data = nullptr ;
	pkt.size = 0 ;
	
	do {
		sl_ffmpeg_decode_packet( decoding_continue, pkt, got_frame, decoder_video, decoder_audio ) ;
	} while ( got_frame && decoding_continue ) ;
}

//////////////////////////////////////////////////////////////////////////

inline const AVRational& sl_ffmpeg_timebaseq()
{
	static const AVRational tbq = { 1, AV_TIME_BASE } ;
	return tbq ;
}

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef __sl_ffmpeg_hpp
