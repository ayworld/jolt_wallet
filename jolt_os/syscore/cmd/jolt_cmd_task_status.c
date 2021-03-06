#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdio.h"

int jolt_cmd_task_status( int argc, char** argv )
{
    /* Get Task Memory Usage */
    char* pcWriteBuffer = malloc( 2048 );
    if( NULL == pcWriteBuffer ) { return -1; }
    vTaskList( pcWriteBuffer );
    printf( "B - Blocked | R - Ready | D - Deleted | S - Suspended\n"
            "Task            Status Priority High    Task #\n"
            "**********************************************\n" );
    printf( pcWriteBuffer );
    free( pcWriteBuffer );
    return 0;
}
