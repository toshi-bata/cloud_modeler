/*
  Copyright 2017 Siemens Product Lifecycle Management Software Inc.
  All rights reserved.
  This software and related documentation are proprietary to
  Siemens Product Lifecycle Management Software Inc.

  Siemens Product Lifecycle Management Software assumes no responsibility
  for the use or reliability of this software; the example frustrum is provided
  in order to run the Parasolid Acceptance Tests and to give application
  writers access to a simple example of a working Frustrum which will
  run on all Parasolid platforms.

  This file defines the frustrum functions which are used by Parasolid for
  PK interface partitioned rollback.
*/

/*
 * ANSI standard files
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

/*
    MS Windows:             cl /I%PARASOLID%
    Unix and Unix-like:     cc -I$PARASOLID
*/

#ifndef _WIN32
/* Define a substitute for a Microsoft function. Note that the return
   types of the Microsoft and traditional functions are different, so
   we can't use the return value. */
#define memcpy_s( dest, dest_size, src, len) memcpy(dest, src, len)
#endif

#include "parasolid_kernel.h"

/*
 * Functions to interface to Parasolid
 */

extern PK_ERROR_code_t FRU_delta_open_for_write(PK_PMARK_t, PK_DELTA_t *);
extern PK_ERROR_code_t FRU_delta_open_for_read(PK_DELTA_t);
extern PK_ERROR_code_t FRU_delta_write(PK_DELTA_t, unsigned, const char *);
extern PK_ERROR_code_t FRU_delta_read(PK_DELTA_t, unsigned, char *);
extern PK_ERROR_code_t FRU_delta_delete(PK_DELTA_t);
extern PK_ERROR_code_t FRU_delta_close(PK_DELTA_t);

/*
 * Application's personal initialization
 */

extern int FRU__delta_init( int action );

#define block_size 1024

typedef struct block_s *block_p_t;
struct block_s {
    size_t used;
    block_p_t next;
    char data[block_size];
};
typedef struct block_s block_t;

typedef struct delta_s {
    PK_PMARK_t pmark;
    int open;
    int read;
    block_p_t first_block;
    block_p_t current_block;
    unsigned int offset;
} delta_t, *delta_p_t;

static unsigned int n_deltas_g = 0;
static delta_p_t *deltas_g;
static int active_g = 0;

static void free_delta( int key )
{
    block_p_t block;
    block_p_t next_block;

    if (deltas_g[key] == NULL)
    {
        printf( "*** free_delta(): Delta %d isn't there!\n", key );
    }

    block = deltas_g[key]->first_block;
    while (block != NULL)
    {
        next_block = block->next;
        free( (void *) block );
        block = next_block;
    }
    free( (void *) (deltas_g[key]) );
    deltas_g[key] = NULL;
    return;
}

int FRU__delta_init( int action )
{
    int res = 0;
    unsigned int i;

    switch (action)
    {
    case 1:
        if (active_g)
        {
            printf( "*** FRU__delta_init(): " );
            printf( "Attempt to start when running!\n" );
            res = 0;
        }
        else
        {
            n_deltas_g = 10;
            deltas_g =
            (delta_p_t *) malloc( (size_t) (n_deltas_g * sizeof( delta_p_t )));
            if (deltas_g == NULL)
            {
                res = 0;
                break;
            }
            i = 0;
            for ( ; i < n_deltas_g; i++ ) deltas_g[i] = NULL;
            active_g = 1;
            res = 1;
        }
        break;
    case 2:
        if (!active_g)
        {
            printf( "*** FRU__delta_init(): " );
            printf( "Attempt to stop when not running!\n" );
            res = 0;
        }
        else
        {
            i = 0;
            for ( ; i < n_deltas_g; i++ )
            {
                if (deltas_g[i] != NULL)
                free_delta( i );
            }
            free( (void *) deltas_g );
            deltas_g = NULL;
            active_g = 0;
            res = 1;
        }
        break;
    default:
        printf( "*** FRU__delta_init(): Invalid argument: %d.\n", action );
        res = 0;
        break;
    }
    return res;
}

PK_ERROR_code_t FRU_delta_open_for_write( PK_PMARK_t pmark, PK_DELTA_t *key )
{
    delta_p_t delta;
    PK_DELTA_t i;

    *key = 0;

    i = (PK_DELTA_t) 0;
    for ( ; i < n_deltas_g; i++ )
    {
        if (deltas_g[i] == NULL)
        {
             *key = i + 1;
             break;
        }
    }
    if (*key == 0)
    {
        deltas_g = (delta_p_t *) (realloc( (void *) deltas_g, (size_t) (
            n_deltas_g * 2 * sizeof( delta_p_t )) ));
        if (deltas_g == NULL)
        {
            printf( "*** FRU_delta_open_for_write(): " );
            printf( "Failed to enlarge delta array!\n" );
            return PK_ERROR_memory_full;
        }
        i = n_deltas_g;
        for ( ; i < 2 * n_deltas_g; i++ ) deltas_g[i] = NULL;
        *key = n_deltas_g + 1;
        n_deltas_g *= 2;
    }
    deltas_g[*key - 1] =
        (delta_p_t) (malloc( (size_t) (sizeof( struct delta_s ))));
    delta = deltas_g[*key - 1];
    if (delta == NULL)
    {
     printf( "*** FRU_delta_open_for_write(): Failed to allocate delta!\n" );
     *key = 0;
     return PK_ERROR_memory_full;
    }
    delta->pmark = pmark;
    delta->open = 1;
    delta->read = 0;
    delta->first_block = NULL;
    delta->current_block = NULL;
    delta->offset = 0;
    return PK_ERROR_no_errors;
}

PK_ERROR_code_t FRU_delta_open_for_read( PK_DELTA_t key )
{
    delta_p_t delta;

    if (key > n_deltas_g || key <= 0)
    {
     printf( "*** FRU_delta_open_for_read(): Key value %d out of range!\n",
             key);
     return PK_ERROR_bad_key;
    }

    delta = deltas_g[key - 1];
    if (delta == NULL)
    {
        printf( "*** FRU_delta_open_for_read(): Delta %d does not exist\n",
                key );
        return PK_ERROR_bad_key;
    }
    if (delta->open)
    {
        printf( "*** FRU_delta_open_for_read(): Delta %d is already open\n",
                key );
        return PK_ERROR_bad_key;
    }
    delta->open = 1;
    delta->read = 1;
    delta->current_block = delta->first_block;
    delta->offset = 0;
    return PK_ERROR_no_errors;
}

PK_ERROR_code_t FRU_delta_write( PK_DELTA_t key, unsigned n_bytes, const char *bytes)
{
    size_t n_copy, offset;
    delta_p_t delta;

    if (key > n_deltas_g || key <= 0)
    {
        printf( "*** FRU_delta_write(): Key value %d out of range!\n", key );
        printf( "*** FRU_delta_write(): n_deltas_g = %d \n", n_deltas_g);
        return PK_ERROR_bad_key;
    }

    delta = deltas_g[key - 1];
    if (delta == NULL)
    {
        printf( "*** FRU_delta_write(): Delta %d does not exist.\n", key );
        return PK_ERROR_bad_key;
    }
    if (!delta->open)
    {
        printf( "*** FRU_delta_write(): Delta %d is not open.\n", key );
        return PK_ERROR_bad_key;
    }
    if (delta->read)
    {
        printf( "*** FRU_delta_write(): Delta %d is open for reading\n", key );
        return PK_ERROR_bad_key;
    }
    if (delta->current_block == NULL)
    {
        delta->first_block =
            (block_p_t) malloc( (size_t) (sizeof( struct block_s)) );
        if (delta->first_block == NULL)
        {
            printf( "*** FRU_delta_write(): Can't allocate block\n" );
            return PK_ERROR_memory_full;
        }
        delta->first_block->used = 0;
        delta->first_block->next = NULL;
        delta->current_block = delta->first_block;
        delta->offset = 0;
    }
    offset = 0;
    for ( ; n_bytes > offset; offset += n_copy )
    {
     if (delta->current_block->used == block_size)
     {
         delta->current_block->next = (block_p_t) malloc( (size_t) (sizeof(
                                      struct block_s )) );
         if (delta->current_block->next == NULL)
         {
             printf( "*** FRU_delta_write(): " );
             printf( "Couldn't allocate following block.\n" );
             return PK_ERROR_memory_full;
         }
         delta->current_block = delta->current_block->next;
         delta->current_block->used = 0;
         delta->current_block->next = NULL;
     }
     n_copy = ((block_size - delta->current_block->used) < (n_bytes - offset) ?
        (block_size - delta->current_block->used) : (n_bytes - offset));

     memcpy_s(
        (void *) (delta->current_block->data + delta->current_block->used),
        (size_t) (block_size - delta->current_block->used),
        (void *) (bytes + offset),
        (size_t) n_copy );

     delta->current_block->used += n_copy;
    }
    return PK_ERROR_no_errors;
}

PK_ERROR_code_t FRU_delta_read( PK_DELTA_t key, unsigned n_bytes, char *bytes)
{
    int n_copy;
    delta_p_t delta;
    unsigned int offset;

    if (key > n_deltas_g || key <= 0)
    {
        printf( "*** FRU_delta_read(): Key value %d out of range!\n", key );
        return PK_ERROR_bad_key;
    }
    delta = deltas_g[key - 1];
    if (delta == NULL)
    {
        printf( "*** FRU_delta_read(): Delta %d does not exist.\n", key );
        return PK_ERROR_bad_key;
    }
    if (!delta->open)
    {
        printf( "*** FRU_delta_read(): Delta %d is not open.\n", key );
        return PK_ERROR_bad_key;
    }
    if (!delta->read)
    {
        printf( "*** FRU_delta_read(): Delta %d is open for writing\n", key );
        return PK_ERROR_bad_key;
    }

    offset = 0;
    for ( ; n_bytes > offset; offset += n_copy )
    {
        if (delta->offset == block_size)
        {
            delta->current_block = delta->current_block->next;
            delta->offset = 0;
            if (delta->current_block == NULL)
            {
                printf( "*** FRU_delta_read(): ");
                printf( "Attempt to read beyond end of delta.\n" );
                return PK_ERROR_file_read_corruption;
            }
        }
        n_copy = ((n_bytes - offset) > (block_size - delta->offset)
                     ? (block_size - delta->offset)
                     : (n_bytes - offset));
        if (bytes != NULL)
        {
            memcpy_s(
                (void *) (bytes + offset),
                (size_t) (n_bytes - offset),
                (void *) (delta->current_block->data + delta->offset),
                (size_t) n_copy );
        }
        delta->offset += n_copy;
    }
    return PK_ERROR_no_errors;
}

PK_ERROR_code_t FRU_delta_delete( PK_DELTA_t key )
{
    delta_p_t delta;

    if (key > n_deltas_g || key <= 0)
    {
        printf( "*** FRU_delta_delete(): Key value %d out of range!\n", key );
        return PK_ERROR_bad_key;
    }

    delta = deltas_g[key - 1];
    if (delta == NULL)
    {
        printf( "*** FRU_delta_delete(): Delta %d does not exist.\n", key );
        return PK_ERROR_bad_key;
    }
    if (delta->open)
    {
        printf( "*** FRU_delta_delete(): Delta %d is open.\n", key );
        return PK_ERROR_bad_key;
    }
    free_delta( (int) (key - 1) );
    return 0;
}

PK_ERROR_code_t FRU_delta_close( PK_DELTA_t key )
{
    delta_p_t delta;

    if (key > n_deltas_g || key <= 0)
    {
        printf( "*** FRU_delta_close(): Key value %d out of range!\n", key );
        return PK_ERROR_bad_key;
    }

    delta = deltas_g[key - 1];
    if (delta == NULL)
    {
        printf( "*** FRU_delta_close(): Delta %d does not exist.\n", key );
        return PK_ERROR_bad_key;
    }
    if (!delta->open)
    {
        printf( "*** FRU_delta_close(): Delta %d is not open\n", key );
        return PK_ERROR_bad_key;
    }
    if (delta->read && delta->first_block != NULL
        && (delta->current_block->next != NULL
        || delta->current_block->used != delta->offset))
    {
        printf( "*** FRU_delta_close(): Delta %d closed with data unread\n",
                key );
    }
    delta->open = 0;
    return 0;
}
