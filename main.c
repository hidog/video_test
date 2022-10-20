#include <stdio.h>
#include "media.h"

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>


int main(int argc, char *argv[])
{
#if 0
	if( argc < 3 )	
	{
		fprintf( stderr, "./video_test input_file output_file" );
		return 0;
	}

	test(argc, argv);
#endif

	char input_file[100] = "/home/hidog/code/video_test/test.mp4";

	AVFormatContext *fmt_ctx = NULL;

    if( avformat_open_input( &fmt_ctx, input_file, NULL, NULL ) < 0 ) 
    {
        fprintf( stderr, "open file %s fail.\n", input_file );
        return 0;
    }

    if( avformat_find_stream_info( fmt_ctx, NULL ) < 0 ) 
    {
        fprintf( stderr, "find stream info fail.\n" );
        exit(1);
    }

    int video_stream_idx = 0;
    AVCodecContext *video_dec_ctx = NULL;

    if( open_codec_context( &video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO ) >= 0 ) 
    {
        // video_stream = fmt_ctx->streams[video_stream_idx];
        printf( "video_stream_idx = %d\n", video_stream_idx );
    }

    int audio_stream_idx;
    AVCodecContext *audio_dec_ctx = NULL;
    if( open_codec_context( &audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO ) >= 0 )
    {
        //audio_stream = fmt_ctx->streams[audio_stream_idx];
        printf( "audio_stream_idx = %d\n", audio_stream_idx );
    }

	printf("test\n");
	return 0;
}
