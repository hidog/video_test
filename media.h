#ifndef MEDIA_H
#define MEDIA_H

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>


#define SUCCESS 1
#define ERROR -1




struct DecodeData_t
{
    int     video_index;
    int     audio_index;
    AVFormatContext *fmt_ctx;
    AVCodecContext  *audio_dec_ctx;
    AVStream        *video_stream; 
    AVStream        *audio_stream;
    AVFrame     *frame;
    AVPacket    *pkt;
} typedef DecodeData;



int open_input( char *filename, DecodeData *dec_data );
int audio_decode( DecodeData *dec_data );


int enc_test();




#endif