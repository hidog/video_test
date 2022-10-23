#ifndef MEDIA_H
#define MEDIA_H

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>


#define  SUCCESS     1
#define  ERROR       -(__LINE__)
#define  NO_FRAME    0


struct Decode_t
{
   int   video_index;
   int   audio_index;
   AVFrame     *frame;
   AVPacket    *pkt;
   AVFormatContext   *fmt_ctx;
   AVCodecContext    *audio_ctx;
   AVStream          *video_stream; 
   AVStream          *audio_stream;
} typedef Decode;


struct Encode_t
{
   AVFormatContext   *fmt_ctx;
   AVCodecContext    *audio_ctx;
   AVStream    *audio_stream;
   AVStream    *video_stream;
   AVPacket    *pkt;
   AVFrame     *frame;
   SwrContext  *swr_ctx;
   int   sample_count;
} typedef Encode;


struct FifoBuffer_t
{
   AVAudioFifo *fifo;
   uint8_t  **tmp_buffer;
   int   input_nb_samples;
   int   output_nb_samples;
} typedef FifoBuffer;


// decode
int   open_input( char *filename, Decode *dec );
int   audio_decode( Decode *dec );

// encode
int   open_output( char *filename, Decode dec, Encode *enc );
int   audio_encode( Encode *enc );
int   write_audio_frame( Decode dec, Encode *enc, FifoBuffer fifobuf );
int   flush_audio( Decode dec, Encode *enc, FifoBuffer fifobuf );

// fifo
int init_fifo( Decode dec, Encode enc, FifoBuffer *fifobuf );
int push_audio_frame(  Encode enc, FifoBuffer fifobuf, AVFrame *dec_audio_frame );
int pop_audio_frame( Encode enc, FifoBuffer fifobuf );

#endif