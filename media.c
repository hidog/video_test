#include "media.h"

#include <assert.h>

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>


static enum AVPixelFormat pix_fmt;

static const char *src_filename = NULL;
static const char *audio_dst_filename = NULL;
static FILE *audio_dst_file = NULL;
static int audio_frame_count = 0;




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
    AVCodecContext  *video_ctx  =   NULL,   *audio_ctx  =   NULL;
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
    //dec->video_ctx =   video_ctx;

    return SUCCESS;
}








































#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

// a wrapper around a single output AVStream
typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    AVPacket *tmp_pkt;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

static int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                       AVStream *st, AVFrame *frame, AVPacket *pkt)
{
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame to the encoder: %s\n",
                av_err2str(ret));
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding a frame: %s\n", av_err2str(ret));
            exit(1);
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(pkt, c->time_base, st->time_base);
        pkt->stream_index = st->index;

        /* Write the compressed frame to the media file. */
        log_packet(fmt_ctx, pkt);
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
         * its contents and resets pkt), so that no unreferencing is necessary.
         * This would be different if one used av_write_frame(). */
        if (ret < 0) {
            fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
            exit(1);
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}

/* Add an output stream. */
static void add_stream(OutputStream *ost, AVFormatContext *oc,
                       const AVCodec **codec,
                       enum AVCodecID codec_id)
{
    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    ost->tmp_pkt = av_packet_alloc();
    if (!ost->tmp_pkt) {
        fprintf(stderr, "Could not allocate AVPacket\n");
        exit(1);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        av_channel_layout_copy(&c->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);
        ost->st->time_base = (AVRational){ 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 400000;
        /* Resolution must be a multiple of two. */
        c->width    = 352;
        c->height   = 288;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        c->time_base       = ost->st->time_base;

        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
        break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* audio output */

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  const AVChannelLayout *channel_layout,
                                  int sample_rate, int nb_samples)
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



#if 0
static void open_audio(AVFormatContext *oc, const AVCodec *codec,
                       OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;

    c = ost->enc;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* init signal generator */
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame     = alloc_audio_frame(c->sample_fmt, &c->ch_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, &c->ch_layout,
                                       c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }

    /* create resampler context */
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        exit(1);
    }

    /* set options */
    av_opt_set_chlayout  (ost->swr_ctx, "in_chlayout",       &c->ch_layout,      0);
    av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_chlayout  (ost->swr_ctx, "out_chlayout",      &c->ch_layout,      0);
    av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        exit(1);
    }
}
#endif






/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
static AVFrame *get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) > 0)
        return NULL;

    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->ch_layout.nb_channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
}

/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
static int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVFrame *frame;
    int ret;
    int dst_nb_samples;

    c = ost->enc;

    frame = get_audio_frame(ost);

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                        c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
        frame = ost->frame;

        frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    return write_frame(oc, c, ost->st, frame, ost->tmp_pkt);
}

/**************************************************************/
/* video output */





static AVFrame *get_video_frame(OutputStream *ost)
{
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, c->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) > 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* as we only generate a YUV420P picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,
                        "Could not initialize the conversion context\n");
                exit(1);
            }
        }
        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx, (const uint8_t * const *) ost->tmp_frame->data,
                  ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
                  ost->frame->linesize);
    } else {
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
static int write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    return write_frame(oc, ost->enc, ost->st, get_video_frame(ost), ost->tmp_pkt);
}

static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    av_packet_free(&ost->tmp_pkt);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}

/**************************************************************/
/* media file output */







int open_output( char *filename,  Decode dec, Encode *enc )
{
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext  *audio_ctx = NULL, *video_ctx = NULL;
    AVStream *audio_stream = NULL, *video_stream = NULL;


    avformat_alloc_output_context2( &fmt_ctx, NULL, NULL, filename );
    if( NULL == fmt_ctx )
    {
        fprintf( stderr, "open output file %s fail.\n", filename );
        return ERROR;
    }




    const AVCodec *audio_codec = NULL, *video_codec = NULL;

    int i, ret;

    int64_t duration_per_frame;
  









    

    // video
    /* find the encoder */
#if 0
    video_codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    if ( NULL == video_codec ) 
    {
        fprintf(stderr, "Could not find encoder for\n");
        exit(1);
    }

    video_stream = avformat_new_stream( fmt_ctx, NULL);
    if ( NULL == video_stream ) 
    {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }

    video_stream->id = fmt_ctx->nb_streams-1;
    video_ctx = avcodec_alloc_context3(video_codec);
    if ( NULL == video_ctx ) 
    {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }

    video_ctx->codec_id = AV_CODEC_ID_HEVC;
    
    video_ctx->bit_rate = 40000000;
    /* Resolution must be a multiple of two. */
    video_ctx->width    = 1920;
    video_ctx->height   = 1080;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    video_stream->time_base = (AVRational){ 1001, 24000 };
    video_ctx->time_base       = video_stream->time_base;
    
    video_ctx->gop_size      = dec_data.video_dec_ctx->gop_size; // 12; /* emit one intra frame every twelve frames at most */
    video_ctx->pix_fmt       = STREAM_PIX_FMT;


    /* Some formats want stream headers to be separate. */
    if ( fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        video_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
#else
    video_stream = avformat_new_stream( fmt_ctx, NULL);
    video_stream->id = fmt_ctx->nb_streams-1;
    //video_stream->time_base.num = 1001;
    //video_stream->time_base.den = 24000;

    ret = avcodec_parameters_copy( video_stream->codecpar, dec.video_stream->codecpar );


    //video_stream->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
    //video_ctx->time_base       = video_stream->time_base;

    //video_stream->time_base.num = dec_data.video_stream->r_frame_rate.den; // (AVRational){ 1001, 24000 };
    //video_stream->time_base.den = dec_data.video_stream->r_frame_rate.num; // (AVRational){ 1001, 24000 };

    //duration_per_frame  =  av_rescale( AV_TIME_BASE, dec_data.video_stream->r_frame_rate.den, dec_data.video_stream->r_frame_rate.num );

#endif


    duration_per_frame  =  av_rescale( AV_TIME_BASE, 1001, 24000 );

    // open video
    /* copy the stream parameters to the muxer */
    //video_ctx = avcodec_alloc_context3(video_codec);

    //ret     =   avcodec_parameters_to_context( video_ctx, dec_data.video_stream->codecpar );
    //ret     =   avcodec_parameters_from_context( video_stream->codecpar, video_ctx );


    //ret     =   avcodec_parameters_to_context( video_ctx, dec_data.video_stream->codecpar );
    //video_stream->time_base.den     =   dec_data.video_dec_ctx->time_base.num; 
    //video_stream->time_base.num     =   dec_data.video_dec_ctx->time_base.den; 
    //video_stream = dec_data.video_stream;


    /*if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }*/














      
    // audio
    /* find the encoder */
#if 1
    audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if ( NULL == audio_codec ) 
    {
        fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(AV_CODEC_ID_AAC));
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

    audio_ctx->sample_fmt  = audio_codec->sample_fmts ? audio_codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    audio_ctx->bit_rate    = 192000;
    audio_ctx->sample_rate = 48000;

    av_channel_layout_copy(&audio_ctx->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);
    audio_stream->time_base = (AVRational){ 1, audio_ctx->sample_rate };
 


    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        audio_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


    ret = avcodec_open2( audio_ctx, audio_codec, NULL );

#else
    audio_stream = avformat_new_stream(fmt_ctx, NULL);
    audio_stream->id = fmt_ctx->nb_streams - 1;
    audio_stream->time_base = dec_data.audio_dec_ctx->time_base; //(AVRational){ 1, 48000 };
#endif


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
    
    //int ret;

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
    frame = alloc_audio_frame( audio_ctx->sample_fmt, &audio_ctx->ch_layout, audio_ctx->sample_rate, 1024);





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



    enc->fmt_ctx = fmt_ctx;
    enc->audio_ctx = audio_ctx;
    //enc->video_ctx = dec.video_ctx; //  video_ctx;
    enc->audio_stream = audio_stream;
    enc->video_stream = video_stream;
    enc->duration_per_frame = duration_per_frame;
    enc->duration_count = 0;
    enc->pkt = pkt;
    enc->frame = frame;
    enc->swr_ctx = swr_ctx;

    enc->dec_video_stream = dec.video_stream;

    return SUCCESS;
}





int audio_encode( Encode enc, AVFrame *audio_frame )
{
    int ret;


    int64_t delay = swr_get_delay( enc.swr_ctx, enc.audio_ctx->sample_rate);
    int dst_nb_samples = av_rescale_rnd( delay + audio_frame->nb_samples,
                                         enc.audio_ctx->sample_rate, enc.audio_ctx->sample_rate, AV_ROUND_UP);
    assert(dst_nb_samples == audio_frame->nb_samples);

    /*ret = av_frame_make_writable(enc.frame);
    if (ret < 0)
        exit(1);*/


    ret = swr_convert( enc.swr_ctx,
                       enc.frame->data, dst_nb_samples,
                       (const uint8_t **)audio_frame->data, audio_frame->nb_samples);
    if (ret < 0) {
        fprintf(stderr, "Error while converting\n");
        exit(1);
    }
    //frame = ost->frame;

    static int sample_count = 0;

    enc.frame->pts = av_rescale_q( sample_count, (AVRational){1, enc.audio_ctx->sample_rate}, enc.audio_stream->time_base);
    sample_count += dst_nb_samples;









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
    
    /* Write the compressed frame to the media file. */
    //log_packet(fmt_ctx, pkt);
    //ret = av_interleaved_write_frame( enc_data.fmt_ctx, enc_data.pkt );
    /* pkt is now blank (av_interleaved_write_frame() takes ownership of
     * its contents and resets pkt), so that no unreferencing is necessary.
     * This would be different if one used av_write_frame(). */
    //if (ret < 0) 
    //{
      //  fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
//        exit(1);
  //  }
    

    return ret;
}






int enc_test()
{
#if 0
    OutputStream video_st = { 0 }, audio_st = { 0 };
    const char *filename;
    const AVCodec *audio_codec = NULL, *video_codec = NULL;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = NULL;
    int i;




    //if (have_audio)
      //  open_audio(oc, audio_codec, &audio_st, opt);



    while (encode_video || encode_audio) 
    {
        /* select the stream to encode */
        if (encode_video &&
            (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                            audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
            encode_video = !write_video_frame(oc, &video_st);
        } else {
            encode_audio = !write_audio_frame(oc, &audio_st);
        }
    }

    av_write_trailer(oc);

    /* Close each codec. */
    if (have_video)
        close_stream(oc, &video_st);
    if (have_audio)
        close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);
#endif
    return 0;
}
