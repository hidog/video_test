#ifndef MEDIA_H
#define MEDIA_H

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>


#define SUCCESS 1
#define ERROR -1




struct DecodeData_t
{
    int     video_index;
    int     audio_index;
    AVFormatContext *fmt_ctx;
    AVCodecContext  *audio_dec_ctx;
    AVCodecContext  *video_dec_ctx;
    AVStream        *video_stream; 
    AVStream        *audio_stream;
    AVFrame     *frame;
    AVPacket    *pkt;
} typedef DecodeData;




struct EncodeData_t
{
    AVFormatContext *fmt_ctx;
    AVCodecContext  *audio_ctx;
    AVCodecContext *video_ctx;
    AVStream *dec_video_stream;
    AVStream *audio_stream;
    AVStream *video_stream;
    int64_t     duration_per_frame;
    int64_t     duration_count;
    AVPacket    *pkt;
    AVFrame     *frame;
    SwrContext  *swr_ctx;
} typedef EncodeData;



int open_input( char *filename, DecodeData *dec_data );

int audio_decode( DecodeData *dec_data );


int open_output( char *filename, DecodeData dec_data, EncodeData *enc_data );

int audio_encode( EncodeData enc_data, AVFrame *audio_frame );


int enc_test();




#endif