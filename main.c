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
    Decode  dec;

    ret =   open_input( "D:\\code\\input.mp4", &dec );
    if( ret < 0 )
    {
        fprintf( stderr, "open input fail.\n" );
        return 0;
    }

    Encode enc;
    open_output( "D:\\code\\output.mp4", dec, &enc );


    while( av_read_frame( dec.fmt_ctx, dec.pkt ) >= 0 )
    {
        if ( dec.pkt->stream_index == dec.audio_index )
            ret =   audio_decode( &dec );


        if( dec.pkt->stream_index == dec.audio_index )
        {
            int ret = audio_encode( enc, dec.frame );

            av_frame_unref(dec.frame);

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {}
            else
            {
                av_packet_rescale_ts( enc.pkt, enc.audio_ctx->time_base, enc.audio_stream->time_base);
                enc.pkt->stream_index = enc.audio_stream->index;
                int ret2 = av_interleaved_write_frame( enc.fmt_ctx, enc.pkt );
            }
        }
        else
        {
            dec.pkt->stream_index = enc.video_stream->index;
            av_packet_rescale_ts( dec.pkt, dec.video_stream->time_base, enc.video_stream->time_base  );
            int ret2 = av_interleaved_write_frame( enc.fmt_ctx, dec.pkt );
        }

        av_packet_unref( dec.pkt );
        if (ret < 0)
            break;
    }

    // flush
    // decode_packet( &dec_data );

    printf("Demuxing succeeded.\n");


    av_write_trailer( enc.fmt_ctx );


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
