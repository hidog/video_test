#include "media.h"

#include <assert.h>

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libavutil/audio_fifo.h>



int     audio_decode( Decode *dec )
{  
    int ret = 0;

    ret = avcodec_send_packet( dec->audio_ctx, dec->pkt );

    if( ret < 0 )
    {
        fprintf( stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }

    if( ret >= 0 )
    {
        ret = avcodec_receive_frame( dec->audio_ctx, dec->frame );
        if (ret < 0) 
        {
            if ( !(ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) )
                fprintf( stderr, "error\n");
        }

        //ret =   output_audio_frame( dec_data->frame );
        //av_frame_unref(dec_data->frame);
    }

    return 0;
}





int open_input( char *filename, Decode *dec )
{
    AVFormatContext *fmt_ctx        =   NULL;    
    int             video_index     =   -1,     audio_index     =   -1;
    AVStream        *video_stream   =   NULL,   *audio_stream   =   NULL;
    AVCodecContext  *audio_ctx  =   NULL;
    AVFrame         *frame  =   NULL;
    AVPacket        *pkt    =   NULL;

    const AVCodec *codec  =   NULL;    
    int ret;

    if( avformat_open_input( &fmt_ctx, filename, NULL, NULL ) < 0 )
    {
        fprintf( stderr, "open %s fail.\n", filename);
        return ERROR;
    }

    if( avformat_find_stream_info( fmt_ctx, NULL ) < 0 ) 
    {
        fprintf( stderr, "find stream info fail.\n" );
        return ERROR;
    }

    audio_index =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0 );
    if( audio_index < 0 ) 
    {
        fprintf( stderr, "This media file has no audio stream.\n" );
        return ERROR;
    } 
    else 
    {
        audio_stream    =   fmt_ctx->streams[audio_index];
        codec =   avcodec_find_decoder( audio_stream->codecpar->codec_id );
        if( NULL == dec )
        {
            fprintf( stderr, "find audio codec fail.\n" );
            return ERROR;
        }

        audio_ctx   =   avcodec_alloc_context3(codec);
        if( NULL  == audio_ctx )
        {
            fprintf( stderr, "allocate audio codec ctx fail.\n" );
            return ERROR;
        }

        ret =   avcodec_parameters_to_context( audio_ctx, audio_stream->codecpar);
        if( ret < 0 )
        {
            fprintf( stderr, "copy audio codec parameters to audio decoder ctx fail.\n" );
            return ERROR;
        }

        ret =   avcodec_open2( audio_ctx, codec, NULL );
        if( ret < 0 )
        {
            fprintf( stderr, "open audio codec fail\n" );
            return ERROR;
        }

        audio_ctx->pkt_timebase   =   fmt_ctx->streams[audio_index]->time_base;
    }

    video_index =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0 );
    if( video_index < 0 )
        printf( "NOTE: this file has no video stream.\n" );
    else 
        video_stream    =   fmt_ctx->streams[video_index];

    av_dump_format( fmt_ctx, 0, filename, 0 );

    // allocate frame, packet.
    frame   =   av_frame_alloc();
    if( NULL == frame )
    {
        fprintf( stderr, "allocate dec frame fail.\n");
        return ERROR;
    }

    pkt =   av_packet_alloc();
    if( NULL == pkt )
    {
        fprintf( stderr, "allocate dec pkt fail\n" );
        return ERROR;
    }

    dec->fmt_ctx       =   fmt_ctx;
    dec->frame         =   frame;
    dec->pkt           =   pkt;
    dec->video_index   =   video_index;
    dec->audio_index   =   audio_index;
    dec->video_stream  =   video_stream;
    dec->audio_stream  =   audio_stream;
    dec->audio_ctx =   audio_ctx;

    return SUCCESS;
}





AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt, const AVChannelLayout *channel_layout, int sample_rate, int nb_samples )
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    frame->format = sample_fmt;
    av_channel_layout_copy(&frame->ch_layout, channel_layout);
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }

    return frame;
}







int open_output( char *filename,  Decode dec, Encode *enc )
{
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext  *audio_ctx = NULL; //, *video_ctx = NULL;
    AVStream *audio_stream = NULL, *video_stream = NULL;


    avformat_alloc_output_context2( &fmt_ctx, NULL, NULL, filename );
    if( NULL == fmt_ctx )
    {
        fprintf( stderr, "open output file %s fail.\n", filename );
        return ERROR;
    }

    const AVCodec *audio_codec = NULL; //, *video_codec = NULL;

    int i, ret;

    // video

    video_stream = avformat_new_stream( fmt_ctx, NULL);
    video_stream->id = fmt_ctx->nb_streams - 1;


    ret = avcodec_parameters_copy( video_stream->codecpar, dec.video_stream->codecpar );


    // audio
    audio_codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
    if ( NULL == audio_codec ) 
    {
        fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(AV_CODEC_ID_OPUS));
        return ERROR;
    }

    audio_stream = avformat_new_stream(fmt_ctx, NULL);
    if ( NULL == audio_stream ) 
    {
        fprintf(stderr, "Could not allocate stream\n");
        return ERROR;
    }

    audio_stream->id = fmt_ctx->nb_streams - 1;

    audio_ctx = avcodec_alloc_context3(audio_codec);
    if ( NULL == audio_ctx )
    {
        fprintf(stderr, "Could not alloc an encoding context\n");
        return ERROR;
    }

    audio_ctx->sample_fmt  = audio_codec->sample_fmts[0];
    audio_ctx->bit_rate    = 192000;
    audio_ctx->sample_rate = 48000; //dec.audio_ctx->sample_rate;

    av_channel_layout_copy(&audio_ctx->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);
    audio_stream->time_base = (AVRational){ 1, audio_ctx->sample_rate };
 

       /*if ( audio_codec->supported_samplerates ) 
       {
            audio_ctx->sample_rate = audio_codec->supported_samplerates[0];
            for (i = 0; audio_codec->supported_samplerates[i]; i++) 
            {
                if ( audio_codec->supported_samplerates[i] == 44100)
                    audio_ctx->sample_rate = 44100;
            }
        }*/


    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        audio_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


    ret = avcodec_open2( audio_ctx, audio_codec, NULL );


    // open audio
    /*if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;*/

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context( audio_stream->codecpar, audio_ctx );
    if (ret < 0) 
    {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }


    av_dump_format( fmt_ctx, 0, filename, 1 );

    const AVOutputFormat *fmt = fmt_ctx->oformat;


    /* open the output file, if needed */
    if( ! (fmt->flags & AVFMT_NOFILE) ) 
    {
        ret = avio_open( &fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str(ret));
            return 1;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header( fmt_ctx, NULL );
    if (ret < 0) 
    {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        return 1;
    }

    AVPacket *pkt = NULL;
    pkt =   av_packet_alloc();


    AVFrame *frame = NULL;
    //audio_ctx->frame_size
    frame = alloc_audio_frame( audio_ctx->sample_fmt, &audio_ctx->ch_layout, audio_ctx->sample_rate, audio_ctx->frame_size);



    /* create resampler context */
    struct SwrContext *swr_ctx = NULL;
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        exit(1);
    }

    /* set options */
    av_opt_set_chlayout  (swr_ctx, "in_chlayout",       &dec.audio_ctx->ch_layout,      0);
    av_opt_set_int       (swr_ctx, "in_sample_rate",     dec.audio_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      dec.audio_ctx->sample_fmt, 0);
    av_opt_set_chlayout  (swr_ctx, "out_chlayout",      &audio_ctx->ch_layout,      0);
    av_opt_set_int       (swr_ctx, "out_sample_rate",    audio_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     audio_ctx->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        exit(1);
    }




    AVAudioFifo *fifo = NULL;
    /* Create the FIFO buffer based on the specified output sample format. */

    fifo = av_audio_fifo_alloc( audio_ctx->sample_fmt, audio_ctx->ch_layout.nb_channels, audio_ctx->frame_size );

    if ( NULL == fifo ) 
    {
        fprintf(stderr, "Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }








    enc->fmt_ctx = fmt_ctx;
    enc->audio_ctx = audio_ctx;
    enc->audio_stream = audio_stream;
    enc->video_stream = video_stream;
    enc->pkt = pkt;
    enc->frame = frame;
    enc->swr_ctx = swr_ctx;
    enc->fifo = fifo;

    return SUCCESS;
}





int audio_encode( Encode enc, AVFrame *audio_frame )
{
    int ret;


    int64_t delay = swr_get_delay( enc.swr_ctx, enc.audio_ctx->sample_rate);
    int dst_nb_samples = av_rescale_rnd( delay + audio_frame->nb_samples,
                                         enc.audio_ctx->sample_rate, enc.audio_ctx->sample_rate, AV_ROUND_UP);
    assert(dst_nb_samples == audio_frame->nb_samples);

    ret = av_frame_make_writable(enc.frame);
    if (ret < 0)
        exit(1);



    uint8_t **tmp_sample = calloc( enc.audio_ctx->ch_layout.nb_channels, sizeof(uint8_t*) );
    av_samples_alloc( tmp_sample, NULL, enc.audio_ctx->ch_layout.nb_channels, audio_frame->nb_samples, enc.audio_ctx->sample_fmt, 0);



    ret = swr_convert( enc.swr_ctx,
                       tmp_sample, audio_frame->nb_samples, //dst_nb_samples,
                       (const uint8_t **)audio_frame->data, audio_frame->nb_samples );
    if (ret < 0) {
        fprintf(stderr, "Error while converting\n");
        exit(1);
    }
    //frame = ost->frame;




    
    /* Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples. */

    int fifo_size = av_audio_fifo_size(enc.fifo);
    int error = av_audio_fifo_realloc( enc.fifo, fifo_size + enc.audio_ctx->frame_size );

    if ( error < 0) 
    {
        fprintf(stderr, "Could not reallocate FIFO\n");
        return error;
    }



    /* Store the new samples in the FIFO buffer. */
    int writed_size = av_audio_fifo_write( enc.fifo, (void **)tmp_sample , audio_frame->nb_samples );
    if ( writed_size < enc.audio_ctx->frame_size) 
    {
        fprintf(stderr, "Could not write data to FIFO\n");
        return AVERROR_EXIT;
    }

    av_freep( &tmp_sample[0] );
    
    


    while ( av_audio_fifo_size(enc.fifo) >= enc.audio_ctx->frame_size )
    {
       const int frame_size = FFMIN(av_audio_fifo_size(enc.fifo), enc.audio_ctx->frame_size);
       int data_written;

       /* Read as many samples from the FIFO buffer as required to fill the frame.
        * The samples are stored in the frame temporarily. */
       int read_size = av_audio_fifo_read( enc.fifo, (void **)enc.frame->data, frame_size);
       if ( read_size < frame_size) 
       {
           fprintf(stderr, "Could not read data from FIFO\n");
           return AVERROR_EXIT;
       }




       static int sample_count = 0;

       enc.frame->pts = av_rescale_q( sample_count, (AVRational){1, enc.audio_ctx->sample_rate}, enc.audio_stream->time_base);
       sample_count +=  enc.frame->nb_samples; //dst_nb_samples;


       // send the frame to the encoder
       ret = avcodec_send_frame( enc.audio_ctx, enc.frame );


       if (ret < 0) 
       {
           fprintf(stderr, "Error sending a frame to the encoder: %s\n",
                   av_err2str(ret));
           //exit(1);
       }

       ret = avcodec_receive_packet( enc.audio_ctx, enc.pkt );
       if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
           return ret;
       else if (ret < 0) 
       {
           fprintf(stderr, "Error encoding a frame: %s\n", av_err2str(ret));
           exit(1);
       }
    
       /* rescale output packet timestamp values from codec to stream timebase */
       av_packet_rescale_ts( enc.pkt, enc.audio_ctx->time_base, enc.audio_stream->time_base );
       enc.pkt->stream_index = enc.audio_stream->index;
    

       //av_packet_rescale_ts( enc.pkt, enc.audio_ctx->time_base, enc.audio_stream->time_base);
       //enc.pkt->stream_index = enc.audio_stream->index;
       int ret2 = av_interleaved_write_frame( enc.fmt_ctx, enc.pkt );

    }











    
    return ret;
}

