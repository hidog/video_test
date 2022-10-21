#include <stdio.h>
#include "media.h"

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>


int main(int argc, char *argv[])
{
    int     ret;
    DecodeData  dec_data;

    ret =   open_input( "D:\\code\\input.mp4", &dec_data );
    if( ret < 0 )
    {
        fprintf( stderr, "open input fail.\n" );
        return 0;
    }




    while( av_read_frame( dec_data.fmt_ctx, dec_data.pkt ) >= 0 )
    {
        if ( dec_data.pkt->stream_index == dec_data.audio_index )
            ret =   audio_decode( &dec_data );

        av_packet_unref( dec_data.pkt );
        if (ret < 0)
            break;
    }

    // flush
    // decode_packet( &dec_data );

    printf("Demuxing succeeded.\n");

#if 0
    if (audio_stream) {
        enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
        int n_channels = audio_dec_ctx->ch_layout.nb_channels;
        const char *fmt;

        if (av_sample_fmt_is_planar(sfmt)) {
            const char *packed = av_get_sample_fmt_name(sfmt);
            printf("Warning: the sample format the decoder produced is planar "
                   "(%s). This example will output the first channel only.\n",
                   packed ? packed : "?");
            sfmt = av_get_packed_sample_fmt(sfmt);
            n_channels = 1;
        }

        if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
            goto end;

        printf("Play the output audio file with the command:\n"
               "ffplay -f %s -ac %d -ar %d %s\n",
               fmt, n_channels, audio_dec_ctx->sample_rate,
               audio_dst_filename);
    }
#endif

end:
#if 0
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    if (audio_dst_file)
        fclose(audio_dst_file);
    av_packet_free(&pkt);
    av_frame_free(&frame);
#endif

    //return ret < 0;




    //test();

    //enc_test();

	printf("test\n");
	return 0;
}
