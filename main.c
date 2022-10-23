#include <stdio.h>
#include "media.h"

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>


int main(int argc, char *argv[])
{
   int      ret,  ret2;
   Decode   dec;
   
   ret   =  open_input( "D:\\code\\input.mp4", &dec );
   if( ret < 0 )
   {
      fprintf( stderr, "open input fail at line %d.\n", -ret );
      exit(0);
   }
   
   Encode   enc;
   ret   =  open_output( "D:\\code\\output.mp4", dec, &enc );
   if( ret < 0 )
   {
      fprintf( stderr, "open output fail at line %d.\n", -ret );
      exit(0);
   }
   
   FifoBuffer  fifobuf;
   ret   =  init_fifo( dec, enc, &fifobuf );
   if( ret < 0 )
   {
      fprintf( stderr, "open fifo fail at line %d.\n", -ret );
      exit(0);
   }   
   
   int   count    =  0;
   while( av_read_frame( dec.fmt_ctx, dec.pkt ) >= 0 )
   {
      if( (count++ % 30000) == 0 )
         printf( "read pkt count = %d\n", count );

      // audio
      if( dec.pkt->stream_index == dec.audio_index )
      {
         ret   =  audio_decode( &dec );
         if( ret == SUCCESS )
            write_audio_frame( dec, &enc, fifobuf );
      }
      // video
      else
      {
         dec.pkt->stream_index   =  enc.video_stream->index;
         av_packet_rescale_ts( dec.pkt, dec.video_stream->time_base, enc.video_stream->time_base );
         av_interleaved_write_frame( enc.fmt_ctx, dec.pkt );
      }

      av_packet_unref( dec.pkt );
   }
   
   // flush audio
   flush_audio( dec, &enc, fifobuf );
   close_decode(&dec);

   av_write_trailer( enc.fmt_ctx );
   close_encode(&enc);
   close_fifo(&fifobuf);

   printf("finish.\n");
	return 0;
}
