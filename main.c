#include <stdio.h>
#include "media.h"

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>


Encode *g_enc = NULL;
FILE *fp     =  NULL;// fopen( "D:\\code\\output.pcm", "wb+" );



void  convert_aac_to_opus()
{
   int      ret,  ret2;
   Decode   dec;
   
   ret   =  open_input( "D:\\code\\input3.mp4", &dec );
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

         ret  =  avcodec_receive_frame( dec.audio_ctx, dec.frame );
         if( ret >= 0 )
            printf("!!!");
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
}




void  merge_g711_to_opus()
{
   int      ret,  ret2;
   Decode   video_dec, audio_dec;
   
   ret   =  open_video_input( "D:\\code\\input2.mp4", &video_dec );
   if( ret < 0 )
   {
      fprintf( stderr, "open input fail at line %d.\n", -ret );
      exit(0);
   }

   ret   =  open_g711_input( "D:\\code\\input.g711a", &audio_dec, AV_CODEC_ID_PCM_ALAW, 2, 48000 );
   if( ret < 0 )
   {
      fprintf( stderr, "open input fail at line %d.\n", -ret );
      exit(0);
   }

   Encode   enc;
   ret   =  open_merge_output( "D:\\code\\output.mp4", video_dec, audio_dec, &enc );
   if( ret < 0 )
   {
      fprintf( stderr, "open output fail at line %d.\n", -ret );
      exit(0);
   }

   g_enc = &enc;
   
   FifoBuffer  fifobuf;
   ret   =  init_fifo( audio_dec, enc, &fifobuf );
   if( ret < 0 )
   {
      fprintf( stderr, "open fifo fail at line %d.\n", -ret );
      exit(0);
   }   
   
   int   count    =  0;
   int   read_video, read_audio;
   while( 1 )
   {
      if( (count++ % 30000) == 0 )
         printf( "read pkt count = %d\n", count );

      // audio
      read_audio  =  av_read_frame( audio_dec.fmt_ctx, audio_dec.pkt );
      if( read_audio >= 0 )
      {
         ret   =  audio_decode( &audio_dec );
         av_packet_unref( audio_dec.pkt );
         if( ret == SUCCESS )
            write_audio_frame( audio_dec, &enc, fifobuf );
      }

      // video
      read_video  =   av_read_frame( video_dec.fmt_ctx, video_dec.pkt );
      if( read_video >= 0 )
      {
         video_dec.pkt->stream_index   =  enc.video_stream->index;
         av_packet_rescale_ts( video_dec.pkt, video_dec.video_stream->time_base, enc.video_stream->time_base );
         av_interleaved_write_frame( enc.fmt_ctx, video_dec.pkt );
         av_packet_unref( video_dec.pkt );
      }

      if( read_video < 0 && read_audio < 0 )
         break;
   }

   av_write_trailer( enc.fmt_ctx );

   
#if 0

   // flush audio
   flush_audio( dec, &enc, fifobuf );
   close_decode(&dec);

   av_write_trailer( enc.fmt_ctx );
   close_encode(&enc);
   close_fifo(&fifobuf);

   printf("finish.\n");
#endif
}




int main(int argc, char *argv[])
{
   fp     =  fopen( "D:\\code\\output.pcm", "wb+" );


   convert_aac_to_opus();

   //merge_g711_to_opus();

   //ffplay_test( argc, argv );


	return 0;
}
