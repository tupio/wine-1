/*
 * Server main function
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "object.h"
#include "thread.h"
#include "request.h"

/* command-line options */
int debug_level = 0;
int persistent_server = 0;

unsigned int server_start_ticks = 0;

/* parse-line args */
/* FIXME: should probably use getopt, and add a (more complete?) help option */

static void usage(const char *exeName)
{
    fprintf(stderr, "\nusage: %s [options]\n\n", exeName);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "   -d<n>  set debug level to <n>\n");
    fprintf(stderr, "   -p     make server persistent\n");
    fprintf(stderr, "   -h     display this help message\n");
    fprintf(stderr, "\n");
}

static void parse_args( int argc, char *argv[] )
{
    int i;
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'd':
                if (isdigit(argv[i][2])) debug_level = atoi( argv[i] + 2 );
                else debug_level++;
                break;
            case 'h':
                usage( argv[0] );
                exit(0);
                break;
            case 'p':
                persistent_server = 1;
                break;
            default:
                fprintf( stderr, "Unknown option '%s'\n", argv[i] );
                usage( argv[0] );
                exit(1);
            }
        }
        else
        {
            fprintf( stderr, "Unknown argument '%s'. Your version of wine may be too old.\n", argv[i] );
            usage(argv[0]);
            exit(1);
        }
    }
}

static void sigterm_handler()
{
    exit(1);  /* make sure atexit functions get called */
}

/* initialize signal handling */
static void signal_init(void)
{
    signal( SIGPIPE, SIG_IGN );
    signal( SIGHUP, sigterm_handler );
    signal( SIGINT, sigterm_handler );
    signal( SIGQUIT, sigterm_handler );
    signal( SIGTERM, sigterm_handler );
    signal( SIGABRT, sigterm_handler );
}

/* get server start ticks used to calculate GetTickCount() in Wine clients */
static void get_start_ticks(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    server_start_ticks = (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

int main( int argc, char *argv[] )
{
    parse_args( argc, argv );
    signal_init();
    open_master_socket();
    setvbuf( stderr, NULL, _IOLBF, 0 );

    if (debug_level) fprintf( stderr, "Server: starting (pid=%ld)\n", (long) getpid() );
    get_start_ticks();
    init_registry();
    select_loop();
    close_registry();
    if (debug_level) fprintf( stderr, "Server: exiting (pid=%ld)\n", (long) getpid() );

#ifdef DEBUG_OBJECTS
    close_atom_table();
    dump_objects();  /* dump any remaining objects */
#endif

    return 0;
}
