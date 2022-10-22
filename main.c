#include <stdio.h>
#include "media.h"

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>


int main(int argc, char *argv[])
{
    //encode_example();

    int     ret;
    DecodeData  dec_data;

    ret =   open_input( "D:\\code\\input.mp4", &dec_data );
    if( ret < 0 )
    {
        fprintf( stderr, "open input fail.\n" );
        return 0;
    }

    EncodeData enc_data;
    open_output( "D:\\code\\output.mp4", dec_data, &enc_data );




    while( av_read_frame( dec_data.fmt_ctx, dec_data.pkt ) >= 0 )
    {
        //if ( dec_data.pkt->stream_index == dec_data.audio_index )
          //  ret =   audio_decode( &dec_data );


        if( dec_data.pkt->stream_index == dec_data.audio_index )
        {
            av_packet_rescale_ts( dec_data.pkt, enc_data.audio_ctx->time_base, enc_data.audio_stream->time_base);
            dec_data.pkt->stream_index = enc_data.audio_stream->index;
        }
        else
        {
            dec_data.pkt->stream_index = enc_data.video_stream->index;

            //AVRational stb = enc_data.video_stream->time_base;

            AVRational r = { 1001, 48000 };

            dec_data.pkt->pts    =   1.0 / AV_TIME_BASE * enc_data.duration_count * 48000 / 1001;
	        dec_data.pkt->dts    =   dec_data.pkt->pts;

            AVRational  realtime_duration = { enc_data.duration_per_frame, AV_TIME_BASE };
            AVRational  duration     =   av_div_q( realtime_duration, r );
            dec_data.pkt->duration   =   av_q2d(duration);

            enc_data.duration_count  +=  enc_data.duration_per_frame;

            av_packet_rescale_ts( dec_data.pkt, dec_data.video_dec_ctx->time_base, r );
        }


        int ret2 = av_interleaved_write_frame( enc_data.fmt_ctx, dec_data.pkt );


        av_packet_unref( dec_data.pkt );
        if (ret < 0)
            break;
    }

    // flush
    // decode_packet( &dec_data );

    printf("Demuxing succeeded.\n");


    av_write_trailer( enc_data.fmt_ctx );


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
