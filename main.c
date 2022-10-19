#include <stdio.h>
#include "demuxing_decoding.h"

int main(int argc, char *argv[])
{
	if( argc < 3 )	
	{
		fprintf( stderr, "./video_test input_file output_file" );
		return 0;
	}

	test(argc, argv);

	

	printf("test\n");
	return 0;
}