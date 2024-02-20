/*=============================================================================

  Copyright 2017 Siemens Product Lifecycle Management Software Inc.
  All rights reserved.
  This software and related documentation are proprietary to
  Siemens Product Lifecycle Management Software Inc.

Code last modified : 12 October  1994 (integrated Vax/Unix example frustrum)
                      9 June     1995 (add NT code for filesystem detection)
                      7 April    1997 (change function prototypes)
                     10 April    1997 (add code for Partition xmt/rcv )
                     11 April    1997 (first attempt to support the Mac)
                     12 August   1999 (allow spaces in P_SCHEMA)
                     25 October  2001 (Use %p for printing a FILE*)
                     20 November 2001 (Add support for debug report
                                       functionality)
                     12 March    2004 Remove old Macintosh support.
                     22 October  2009 No plat-specific newline in g_preamble
                     31 October  2014 Make code comply with Microsoft Security
                                      Development Lifecycle, and still work
                                      on non-Windows platforms.
                     02 December 2015 Add minimal support for mesh guises.
                     12 October  2017 Finish initial iOS support.

Siemens Product Lifecycle Management Software assumes no responsibility for
the use or reliability of this software; the example frustrum is provided in
order to run the Parasolid Acceptance Tests and to give application writers
access to a simple example of a working Frustrum which will run on all
Parasolid platforms.

The example code has been written to contain a minimal amount of machine
specific code. Where necessary, such code is delimited by  #if <macro>  #endif
statements, where the macro name is one recognised by the appropriate compiler:

    #ifdef VMS
    #endif

    #ifdef _WIN32           [this is used to enclose Windows NT specific code]
    #endif                  [and is recognised by the Visual C / C++ compiler]

or the name is defined explicitly on the command line when the file is compiled:

i.e. the rs6000 (AIX) version of frustrum_link.com compiles the example as
    cc -c -w  $PARASOLID/frustrum.c   -DPS_AIX  -I$PARASOLID    ... etc ...

The example code includes comments (marked with the words "machine specific")
which suggest where the application writer may wish to alter the example code
to extend the functionality or to improve performance.

Notes:

    As an example, this example frustrum writes the value "unknown" into
    the date and username fields of the file header (a fully functional
    version would write correct keyword/value pairs to the file headers)

    Platforms where size_t is larger than int (for example, WIN64) will
    produce a number of compile warnings for truncations of size_t to
    int. This isn't a problem for this simple frustrum.

=============================================================================*/

/*=============================================================================
                 #INCLUDES

    MS Windows:             cl /I%PARASOLID%
    Unix and Unix-like:     cc -I$PARASOLID
=============================================================================*/

#include "frustrum_ifails.h"
#include "frustrum_tokens.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef PS_AIX
#include <signal.h>
#include <sys/param.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _WIN32
/* Define substitutes for Microsoft functions. Note that the return
   types of the Microsoft and traditional functions are different, so
   we can't use the return value. */
#define memcpy_s( dest, dest_size, src, len) memcpy(dest, src, len)
#define strcpy_s( dest, dest_size, src)      strcpy(dest, src)
#define strcat_s( dest, dest_size, src)      strcat(dest, src)
#endif
/*=============================================================================
                 #DEFINES
=============================================================================*/


/* determines whether information messages are printed */

#define trace_flag 0

#define PS_LONG_NAME  0
#define PS_SHORT_NAME 1

/* extra useful ifails */

#define  FR_not_started       FR_unspecified
#define  FR_internal_error    FR_unspecified

/* Maximum amount of memory to be allocated. iOS has limited available
   memory and reacts very badly to an app trying to use too much, so we set
   a limit that should work on any 64-bit iOS device.

   The limit is applied to all platforms for simplicity, and as a reminder
   that this frustrum should not be used for real applications.
*/

#ifdef PS_IOS
#define MAX_MEMORY_ALLOCATION (256  * 1024 * 1024)
#else
#define MAX_MEMORY_ALLOCATION (2047 * 1024 * 1024)
#endif

/* other useful definitions */

#define null_strid          (-1)
#define max_namelen         255         /* maximum length of a full pathname */
#define max_header_line (max_namelen+32) /* for long FILE=name in the header */
#define max_open_files      32

#define read_access         1
#define write_access        2
#define read_write_access   3

#define end_of_string_c     '\0'
#define end_of_string_s     "\0"

#define new_line_c          '\n'
#define new_line_s          "\n"

#define semi_colon_c        ';'
#define semi_colon_s        ";"

#ifdef _WIN32
#define dir_separator_c     '\\'
#define dir_separator_s     "\\"
#else
#define dir_separator_c     '/'
#define dir_separator_s     "/"
#endif

/*=============================================================================
                 MACROS
=============================================================================*/

#define trace_print \
    if (!trace_flag) /* skip */; else printf


/*=============================================================================
                 STRUCTS
=============================================================================*/


/* one structure per open file containing info such as filename and
   the C stream id. the structures are chained together, accessed via
   the "open_files" variable
*/

typedef struct file_s *file_p;

typedef struct file_s
{
  file_p next;
  file_p prev;
  int    strid;
  int    guise;
  int    format;
  int    access;
  char   name[max_namelen+1];
  char   key[max_namelen+1];
  FILE  *stream;
} file_t;

static file_p open_files = NULL;

/* file stream identifiers and count of open files */
static int stream_id[max_open_files];
static int file_count = 0;

/* frustrum start count (0 not started) */
static int frustrum_started = 0;



/*=============================================================================
                 GLOBAL VARIABLES
=============================================================================*/

/* the following are for writing and checking file headers */

static char g_preamble_1[ max_header_line ] = end_of_string_s;
static char g_preamble_2[ max_header_line ] = end_of_string_s;
static char g_prefix_1[ max_header_line ] = "**PART1;\n";
static char g_prefix_2[ max_header_line ] = "**PART2;\n";
static char g_prefix_3[ max_header_line ] = "**PART3;\n";
static char g_trailer_start[ max_header_line ] = "**END_OF_HEADER";
static char g_trailer[ max_header_line ] = end_of_string_s;
static char g_unknown_value[] = "unknown";

/* machine specific: fopen file open modes. On NT platforms use binary */
/* mode to suppress the writing of carriage returns before line feeds  */

#ifdef _WIN32
static char g_fopen_mode_read_text[]   = "r";
static char g_fopen_mode_read_binary[] = "rb";
static char g_fopen_mode_write[]       = "wb";
static char g_fopen_mode_append[]      = "wb+";
#else
static char g_fopen_mode_read[]        = "r";
static char g_fopen_mode_write[]       = "w";
static char g_fopen_mode_append[]      = "w+";
#endif

/* this buffer used for input-output of file headers and text files */
static char    *input_output_buffer = NULL;
static size_t   input_output_buflen = 0;

/* This is used to keep track of the amount of memory currently allocated.
   On macOS, the OS will use disc space for swap past any sane multiple
   of RAM size, unless restrained, and on iOS, using too much
   memory makes the OS kill the app
*/
static size_t frustrum_memory_allocated = 0;

#ifdef PS_AIX
/* machine specific: the following variable is used on the AIX (RS6000) */
static int short_of_memory = 0;         /* set the global flag as false */
#endif

#ifdef PS_IOS
/* Machine specific: these globals are used to manage paths on iOS and
   similar platforms. */
static char ios_homedir[2*max_namelen]    = { 0 };
static char ios_workdir[max_namelen]      = { 0 };
static char ios_schemas[max_namelen]      = { 0 };
#endif

/*=============================================================================
                 UTILITY FUNCTIONS
=============================================================================*/


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: lowercase

History:

  May 1990 - reformatted for example frustrum code

Description:

  Convert the string to lower case.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static char *lowercase( char* str )
    {
    char ch, *ptr = str;
    while ((ch = *ptr) != end_of_string_c)
        {
        if (isupper( ch )) *ptr = tolower( ch );
        ++ptr;
        }
    return str;
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: format_string

History:

  May 1990 - reformatted for example frustrum code

Description:

  Returns a pointer to a lowercase string which declares the file format
  (binary or text). This is for writing into file headers.


:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static char* format_string( int format )
    {
    static char ffbnry[] = "binary";
    static char fftext[] = "text";
    switch( format )
      {
      case FFBNRY:
        return ffbnry;
      case FFTEXT:
        return fftext;
      }
    return g_unknown_value;
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: guise_string

History:

  May 1990      - reformatted for example frustrum code
  November 2001 - add support for new fileguise (FFCDBG)
  December 2015 - add support for new fileguise (FFCXMM)

Description:

  Returns a pointer to a lowercase string which declares the file guise
  (that is rollback, snapshot, journal, transmit, schema, licence).
  Romulus files do not have headers; guise FFCXMO is  not valid.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static char* guise_string( int guise )
    {
    static char ffcrol[] = "rollback";
    static char ffcsnp[] = "snapshot";
    static char ffcjnl[] = "journal";
    static char ffcxmt[] = "transmit";
    static char ffcxmo[] = "old_transmit";
    static char ffcsch[] = "schema";
    static char ffclnc[] = "licence";
    static char ffcxmp[] = "transmit_partition";
    static char ffcxmd[] = "transmit_deltas";
    static char ffcdbg[] = "debug_report";
    static char ffcxmm[] = "mesh";

    switch ( guise )
      {
      case FFCROL: return ffcrol;
      case FFCSNP: return ffcsnp;
      case FFCJNL: return ffcjnl;
      case FFCXMT: return ffcxmt;
      case FFCSCH: return ffcsch;
      case FFCLNC: return ffclnc;
      case FFCXMP: return ffcxmp;
      case FFCXMD: return ffcxmd;
      case FFCDBG: return ffcdbg;
      case FFCXMM: return ffcxmm;
      }
    return g_unknown_value;
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: nt_short_filesystem

History:

  June      1995 - written
  October   2014 - New string handling

Description:

    machine specific (NT) function to check whether the device implied
    by the root portion of a path  E:\users\me\xx  or \\mc123\share\yy
    or the "currently selected device" implies a short name filesystem.

    ( i.e. should the frustrum use 3 or 7 character file extensions )

    In this implementation, we assume that a FAT (FileAllocationTable)
    filesystem will only support short 8.3 type names. This assumption
    is true for Win32S applications (IX86 only) and pre NT version 3.5
    and where the NT registry option  "Win32FileSystem"
        HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\FileSystem
        has been set explicitly to 1.

    The 'short_filesystem' feature is only relevant if your application
    is ported to IX86 and is required to run under Windows 3.1 / Win32S
    otherwise your application can just use 7 character file extensions

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifdef _WIN32
static int nt_filesystem_type(char* pathname)
                    /* this can be a fully specified file or directory name */
                    /*    (only the first part of the pathname is examined) */
                    /* or it can be blank or null implying "current device" */
    {
    int default_return;
    int res;
    char *root_of_filesystem;
    char root_of_file_system_name[256] = "";
    char filesystemtype[256] = "";
    DWORD maxnamelength;
    DWORD filesystemtypelen;
#ifdef _M_IX86
    /* running on an I386 machine where FAT filesystems are more common */
    default_return = PS_SHORT_NAME;
#else
    /* running on AlphaNT/MipsNT where NTFS filesystems are more common */
    default_return = PS_LONG_NAME;
#endif
    res = default_return;
    root_of_filesystem = 0L;
    maxnamelength = (DWORD) -1;
    filesystemtypelen = (DWORD) 256;
    if (pathname != 0L && strlen( pathname ) > 0)
      {
      /* examine first few characters in pathname  */
      strcpy_s( root_of_file_system_name, sizeof(root_of_file_system_name),
                                                                pathname );
      if ( root_of_file_system_name[0] == dir_separator_c &&
           root_of_file_system_name[1] == dir_separator_c )
        {
        /* look for a UNC name of the form \\<node>\<share>\...         */
        /* constructed filesystem name must must include the trailing   */
        /* backslash after sharename see KnowledgeBase PSS ID Q119219   */
        int i;
        int len;
        int share_found = 0;
        len = (int) (strlen( root_of_file_system_name ));
        for ( i = 2 ; i < len && root_of_filesystem == 0L; i++ )
          {
          if ( root_of_file_system_name[i] == dir_separator_c )
            {
            if ( share_found )
              /* second single backslash marks end of share name  */
              {
              root_of_file_system_name[i + 1] = '\0';
              root_of_filesystem = root_of_file_system_name;
              }
            else
              share_found = 1;
            }
          }
        }
      else
        /* look for a name of form Z:\... or Z:...                      */
        {
        if (root_of_file_system_name[1] == ':')
          {
          /* assume root_of_file_system_name[0] contains drive letter */
          /* (trailing slash makes this a 'root' directory specifier) */
          root_of_file_system_name[2] = dir_separator_c;
          root_of_file_system_name[3] = '\0';
          root_of_filesystem = root_of_file_system_name;
          }
        }
      }

    if (GetVolumeInformation(root_of_filesystem,/* receive: system name     */
                             0L,                /* return : volume name     */
                             (DWORD) 0,         /*                          */
                             0L,                /* return : no serial num   */
                             &maxnamelength,    /* return : name length     */
                             0L,                /* return : no system flags */
                             filesystemtype,    /* return : file system     */
                             filesystemtypelen) == 1 )
      /* GetVolumeInformation function is implemented in WIN32 and Win32S */
      /* so this function is always available. If the pathname is null or */
      /* it doesn't include any device information, GetVolumeInformation  */
      /* returns whether or not the current device is a FAT file system.  */
      /* (GetVolumeInformation returns zero if the device does not exist) */
      {
      if (strcmp( filesystemtype, "FAT" ) == 0)
        res = PS_SHORT_NAME;
      else if (maxnamelength <= 0)
        /* not sure (return default value)  */
        res = default_return;
      else
        {
        /* use maxnamelength to decide  (12 chars in name "abcdefgh.ijk") */
        if ( maxnamelength <= 12 )
          res = PS_SHORT_NAME;
        else
          res = PS_LONG_NAME;
        }
      }
    return res;
    }
#endif

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: filetype_guise_string

History:

  April 1995    - written
  June  1995    - added nt
  November 2001 - added support for FFCDBG guise
  December 2015 - added support for FFCXMM guise

Description:

  Returns a pointer to a filetype string for the specified guise.
  Used in the construction of filenames in FFOPRD, FFOPWR etc.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static char* filetype_guise_string( int guise, int filesystem  )
         /* filesystem should be set to PS_LONG_NAME  (most platforms) */
         /* or                          PS_SHORT_NAME  ( _WIN32 only ) */
    {
    /* machine specific: parts of file types for various guises */

    /* the following are only applicable to NT */
    static char nt_short_ffcsnp[] = ".N";
    static char nt_short_ffcjnl[] = ".J";
    static char nt_short_ffcxmt[] = ".X";
    static char nt_short_ffcsch[] = ".S";
    static char nt_short_ffclnc[] = ".L";
    static char nt_short_ffcxmo[] = ".XMT";
    static char nt_short_ffcxmp[] = ".P";
    static char nt_short_ffcxmd[] = ".D";
    static char nt_short_ffcdbg[] = ".XML";
    static char nt_short_ffcxmm[] = ".M";

    static char ffcsnp[] = ".snp";
    static char ffcjnl[] = ".jnl";
    static char ffcxmt[] = ".xmt";
    static char ffcsch[] = ".sch";
    static char ffclnc[] = ".lnc";
    static char ffcxmo[] = ".xmt";
    static char ffcxmp[] = ".xmp";
    static char ffcxmd[] = ".xmd";
    static char ffcdbg[] = ".xml";
    static char ffcxmm[] = ".xmm";

    switch( guise )
        {
        case FFCSNP:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffcsnp;
            return ffcsnp;
        case FFCJNL:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffcjnl;
            return ffcjnl;
        case FFCXMT:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffcxmt;
            return ffcxmt;
        case FFCSCH:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffcsch;
            return ffcsch;
        case FFCLNC:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffclnc;
            return ffclnc;
        case FFCXMO:
            return ffcxmo;
        case FFCXMP:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffcxmp;
            return ffcxmp;
        case FFCXMD:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffcxmd;
            return ffcxmd;
        case FFCDBG:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffcdbg;
            return ffcdbg;
        case FFCXMM:
            if ( filesystem == PS_SHORT_NAME ) return nt_short_ffcxmm;
            return ffcxmm;
        }

    return end_of_string_s;
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: filetype_format_string

History:

  April 1995 - written
  June  1995 - added nt

Description:

  Returns a pointer to a file format (text or binary) filetype string
  for the specified format.
  Used in the construction of filenames in FFOPRD, FFOPWR etc.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static char* filetype_format_string( int format, int filesystem )
    /* filesystem set to PS_LONG_NAME  (most platforms) */
    /* or                PS_SHORT_NAME  ( _WIN32 only ) */
    {
    /* machine specific: parts of file types for various formats */

    /* the following are only applicable to NT */
    static char nt_short_ffbnry[] = "_B";
    static char nt_short_fftext[] = "_T";

    static char ffbnry[] = "_bin";
    static char fftext[] = "_txt";

    switch ( format )
        {
        case FFBNRY:
            if ( filesystem == PS_SHORT_NAME )return nt_short_ffbnry;
            return ffbnry;
        case FFTEXT:
            if ( filesystem == PS_SHORT_NAME )return nt_short_fftext;
            return fftext;
        }

    return end_of_string_s;
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: filekey_string

History:

  May 1990 - reformatted for example frustrum code
  Oct 1994 - safer string copy
  Oct 2014 - New string handling.

Description:

  Returns a pointer to a string which is a copy of the filename
  which was passed to FFOPRD/FFOPWR (before having any directory
  prefix or file extension added to it). This consists of appending
  a null character to the string.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static char* filekey_string( char* keynam, int keylen )
    {
    static char keyword_value[max_namelen+1] = "";

    if( keylen > max_namelen)
        keylen = max_namelen;

    memcpy_s(keyword_value, sizeof(keyword_value), keynam, keylen);
    keyword_value[keylen] = end_of_string_c;

    return keyword_value;
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: delete_file

History:

  May 1990 - reformatted for example frustrum code
  Oct 1994 - use remove() function on all platforms

Description:

  Deletes the named file.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void delete_file( char* name, int* ifail )
    {
    trace_print(">>> delete_file : \"%s\"\n", name );

    if(remove( name ) != 0)
        {
        *ifail = FR_close_fail;
        trace_print(">>> returning from 'delete_file' with ifail %d\n",
                    *ifail );
        }
    else
        *ifail = FR_no_errors;
    }

#ifdef PS_AIX

/* machine specific: the following function is used on the AIX (RS6000) */
/* >> we recommend you include a similar check in your AIX frustrum. << */

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: danger_catcher

History:

  May 1992 - added for m/c aix

Description:

  Called by the operating system when the condition SIGDANGER occurs
  (implying that a system crash is imminent and which is interpreted
   as meaning that the system is about to run out of virtual memory).
  Resets itself & sets a flag so that FMALLO knows of this condition.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void danger_catcher( int sig )
    {
    signal(SIGDANGER, danger_catcher); /* reminder of action required */
    short_of_memory = 1;               /* set the global flag as true */
    }

#endif

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: implement_getenv

History:

  Oct 2014  Added for new string handling

Description:

  Wrapper round getenv_s, on Windows platforms, or traditional getenv, on
  other platforms, to provide a uniform interface.

  For iOS, we take advantage of the fact that the parasolid_test program
  only uses one environment variable, P_SCHEMA, and just return the path
  to the schemas.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


char *implement_getenv( char *name)
{
    #ifndef _WIN32
    #ifdef PS_IOS
    return ios_schemas;     /* Use global instead of env var on iOS */
    #endif
    return getenv( name);   /* Traditional getenv  */
    #else
    size_t needed, length;
    char workbuff[1024], *mem;
    if( getenv_s( &needed, workbuff, sizeof(workbuff), name) != 0)
        {
        trace_print( "getenv_s failed on %s\n", name);
        return NULL;
        }
    length = strlen( workbuff);
    if( (mem = malloc( length+1)) == NULL)
        {
        trace_print( "getenv_s couldn't allocate memory\n");
        return NULL;
        }
    strcpy_s( mem, length+1, workbuff);
    return mem;
    #endif
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: implement_fopen

History:

  Oct 2014  Added for new string handling
  Oct 2017  Revised to accomodate iOS

Description:

  Wrapper round fopen_s, on Windows platforms, or traditional fopen, on
  other platforms, to provide a uniform interface. On iOS, deals with
  access to the working directory.

  Important iOS note: this implementation puts all files, apart from
  schemas, into the working directory, and requires the schema directory
  to be a sub-directory of the working directory. You may well want
  to use some more sophisticated organisation, possibly using
  different directories for different file guises, or the like.

  Think carefully before using the tmp directory for rollback files on
  iOS. If you use rollback to survive the application being killed and
  restarted, remember that the operating system is at liberty to remove
  files from the tmp directory any time your app is not running.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

FILE *implement_fopen( char *name, char *mode)
{
    #ifdef _WIN32
    FILE *opened_file;
    if( fopen_s( &opened_file, name, mode) != 0)
        {
        trace_print( ">>> fopen_s failed on %s\n", name);
        return NULL;
        }
    return opened_file;
    #else   /* ! _WIN32 */
    #ifdef PS_IOS
    char fullname[max_namelen*4];
    /* Build the full pathname to the file in the working directory. */
    sprintf( fullname, "%s%s%s%s%s",
        ios_homedir, dir_separator_s, ios_workdir, dir_separator_s, name);
    trace_print( ">>> implement_fopen opening %s with mode %s\n",
        fullname, mode);
    return fopen( fullname, mode);
    #else   /* Not _WIN32 or PS_IOS */
    return fopen( name, mode);
    #endif  /* PS_IOS */
    #endif  /* _WIN32 */
}

#ifdef PS_IOS
/* Machine specific: this is used on iOS to store */
/* pathnames for use in file i/o in the sandbox. */
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: implement_ios_paths

History:

  Oct 2017  Added for iOS support

Description:

  Stores the strings it is passed in global static variables, where they are
  used by implement_fopen and extend_schema_filename. Returns 0 for success,
  1 for failure.

  Thread safety: this function should not be called while the modeller
  is running.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int implement_ios_paths( const char *home,
                         const char *work,
                         const char *sch)
{
    if( (home != NULL) &&
        (strlen( home) > 0) &&
        (strlen( home) < ((2*max_namelen)-1)))
        {
        /* trace_print( ">>> Home directory string: %s\n", home); */
        strcpy_s( ios_homedir, sizeof(ios_homedir), home);
        }
    else
        {
        trace_print ( ">>> Home directory string invalid: %s\n", home);
        return 1;
        }

    if( (work != NULL) &&
        (strlen( work) > 0) &&
        (strlen( work) < (max_namelen-1)))
        {
        /* trace_print ( ">>> Work directory string: %s\n", work); */
        strcpy_s( ios_workdir, sizeof(ios_workdir), work);
        }
    else
        {
        trace_print ( ">>> Work directory string invalid: %s\n", work);
        return 1;
        }

    if( (sch != NULL) &&
        (strlen( sch) > 0) &&
        (strlen( sch) < (max_namelen-1)))
        {
        /* trace_print ( ">>> Schema directory string: %s\n", sch); */
        strcpy_s( ios_schemas, sizeof(ios_schemas), sch);
        }
    else
        {
        trace_print ( ">>> Schema directory string invalid: %s\n", sch);
        return 1;
        }

    return 0;
}
#endif /* PS_IOS */

/*=============================================================================
                    FILE HANDLING FUNCTIONS
=============================================================================*/



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: new_open_file

History:

  May 1990 - reformatted for example frustrum code
  October 2001 - changed %d to %p in trace_print of a FILE*
  October 2014 - New string handling, fix size warnings.

Description:

  Allocates new structure, adds the file information and
  adds it to the list of open-file structures.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void new_open_file( FILE* stream, int guise, int format, int access,
                           char* filename, char* keyname, file_p* file_ptr,
                           int* ifail )
    {
    file_p ptr;
    file_p temp;
    int i;

    /* allocate and add file structure into list of open files */
    ptr = (file_p) malloc( sizeof( file_t ));
    if (ptr == NULL)
      {
        fclose( stream );
        {
        *ifail = FR_open_fail;
        trace_print(">>> returning from 'new_open_file' with ifail %d\n",
                    *ifail);
        return;
        }
      }

    if (open_files == NULL)
        open_files = ptr;
    else
      {
        for ( temp = open_files; temp->next != NULL; temp = temp->next )
            /* skip */;
        temp->next = ptr;
      }

    /* initialise file structure */
    ptr->next = NULL;
    if (open_files == ptr)
        ptr->prev = NULL;
    else
        ptr->prev = temp;
    for ( i = 0; stream_id[i] != 0; i++ )
        /* skip */;

    stream_id[i] = i + 1;
    ptr->strid = i + 1;
    ptr->guise = guise;
    ptr->format = format;
    ptr->access = access;
    ptr->stream = stream;

    strcpy_s( ptr->name, sizeof(ptr->name), filename );
    strcpy_s( ptr->key, sizeof(ptr->key), keyname );

    file_count++;

    trace_print(">>> new_open_file - filename: \"%s\"\n",
                ptr->name );
    trace_print(">>> new_open_file - file count : %d; stream %p\n",
                file_count, stream );

    *file_ptr = ptr;
    *ifail = FR_no_errors;
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: write_to_file

History:

  May 1990 - reformatted for example frustrum code
  Oct 1992 - minimise freeing of transfer buffer

Description:

  Writes given buffer to open file as either ascii or binary.
  Uses 'fputs' for ascii and 'fwrite' for binary.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void write_to_file( file_p file_ptr, const char* buffer, int header,
                           size_t buffer_len, int* ifail)
    {
    /* in this example frustrum, the headers of binary and text files are */
    /* output with fputs() and other binary data are output with fwrite() */

    if (header || file_ptr->format == FFTEXT)
        {
        if ( buffer_len == 1 )
            {
            if (fputc( buffer[0], file_ptr->stream ) == EOF)
              {
              *ifail = FR_write_fail;
              trace_print(">>> returning from 'write_to_file' with ifail %d\n",
                          *ifail );
              }
            else
              {
              *ifail = FR_no_errors;
              }
            }
        else
            {
            size_t required;
            int count;
            /* check whether the global input-output buffer is long enough */
            required = (buffer_len + 1) * sizeof( char );

            if ( input_output_buflen < required )
                {
                if ( input_output_buffer != NULL )
                    free(input_output_buffer);

                input_output_buflen = 0;
                input_output_buffer = (char *) malloc( required );

                if ( input_output_buffer == NULL )
                    {
                    *ifail = FR_unspecified;
                    trace_print( ">>> failed to allocate memory" );
                    trace_print( ">>> returning from 'write_to_file' ifail %d\n",
                                 *ifail );
                    return;
                    }
                else
                    input_output_buflen = required;
                }


            /* copy the buffer and add a null-terminating character */

            for( count = 0; count < buffer_len; count ++ )
              {
                input_output_buffer[ count ] = buffer[ count ];
              }
            input_output_buffer[buffer_len] = end_of_string_c;

            /* the string will already contain any necessary formatting characters
              (added by the header routines or Parasolid); fputs does not add any
            */

            if (fputs( input_output_buffer, file_ptr->stream ) == EOF)
              {
                *ifail = FR_write_fail;
                trace_print(">>> returning from 'write_to_file' with ifail %d\n",
                            *ifail );
              }
            else
              {
              *ifail = FR_no_errors;
              }
            }
        }
    else
        {
        /* write to binary file */
        size_t written;
        written = fwrite( buffer, (unsigned) (sizeof(char)),
                          buffer_len, file_ptr->stream );
        if ( written != buffer_len)
          {
            *ifail = FR_write_fail;
            trace_print(">>> returning from 'write_to_file' with ifail %d\n",
                        *ifail );
          }
        else
          {
            *ifail = FR_no_errors;
          }
        }
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: read_from_file

History:

  May 1990 - reformatted for example frustrum code
  Oct 1992 - minimise freeing of transfer buffer
  Oct 2014 - New string handling

Description:

  Read the required amount of data from open file.
  Uses 'fgets' for ascii and 'fread' for binary.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void read_from_file( file_p file_ptr, char* buffer, int header,
                            int max_buffer_len, int* buffer_len, int* ifail)
    {
    /* in this example frustrum, the headers of binary and text files are */
    /* input with fgets()  and  other binary data are input with fread()  */

    if (header || file_ptr->format == FFTEXT)
        {
        if ( max_buffer_len == 1 )
            {
            int value = fgetc(file_ptr->stream);
            if ( value == EOF )
                {
                *ifail = (feof(file_ptr->stream)?FR_end_of_file:FR_read_fail);
                trace_print(
                    ">>> returning from 'read_from_file' with ifail %d\n",
                    *ifail );
                return;
                }
            else
                {
                buffer[0] = value;
                *buffer_len = 1;
                }
            }
        else
            {
            int required;

            /* check whether current global input-output buffer long enough */
            required = (max_buffer_len + 1) * sizeof( char );

            if ( input_output_buflen < required )
                {
                if ( input_output_buffer != NULL )
                    free(input_output_buffer);

                input_output_buflen = 0;
                input_output_buffer = (char *) malloc( required );

                if ( input_output_buffer == NULL )
                    {
                    *ifail = FR_unspecified;
                    trace_print(">>> failed to allocate memory" );
                    trace_print(
                              ">>> returning from 'read_from_file' ifail %d\n",
                                      *ifail);
                    return;
                    }
                else
                    input_output_buflen = required;
                }

            /* note that the second argument to fgets is the maximum number */
            /* of characters which can ever be written (including the null) */
            /* which is why the second argument to fgets = max_buffer_len+1 */
            if (fgets(input_output_buffer, max_buffer_len+1,
                      file_ptr->stream) == NULL)
                {
                *ifail = (feof(file_ptr->stream)?FR_end_of_file:FR_read_fail);

                trace_print(
                    ">>> returning from 'read_from_file' with ifail %d\n",
                    *ifail );
                return;
                }

            /* Copy input buffer back to calling function without terminator
               This requires us to use memcpy_s, as Windows strncpy_s *insists*
               on writing a terminator. */
            *buffer_len = (int)strlen(input_output_buffer);
            memcpy_s(buffer, max_buffer_len, input_output_buffer, *buffer_len);
            }
        }
    else
        {
        size_t chars = fread( buffer, (unsigned) (sizeof( char )),
                                max_buffer_len, file_ptr->stream );
        if (chars == 0)
          {
          *ifail = ( feof(file_ptr->stream) ? FR_end_of_file : FR_read_fail );

          trace_print(">>> returning from 'read_from_file' with ifail %d\n",
            *ifail );
          return;
          }

        *buffer_len = (int)chars;
        }

    /***
    trace_print(">>> %d bytes read\n", *buffer_len );
    ***/

    *ifail = FR_no_errors;
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: check_valid_filename

History:

  May 1990 - reformatted for example frustrum code
  Aug 1999 - make an initial space be an invalid filename otherwise
             spaces are ok.

Description:

  Checks that filename is valid, that no spurious characters are there

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void check_valid_filename( char* filename, int* ifail )
    {
    size_t len;
    int i;

    /* check that the string is not too long */
    len = strlen( filename );
    if ( len > max_namelen )
      {
        *ifail = FR_bad_name;
        trace_print(">>> returning from 'check_valid_filename' ifail %d\n",
                    *ifail );
        return;
      }

    /* machine specific:
       The checks made here must be able to trap 'bad' filename
       strings generated by FTMKEY i.e. FTMKEY deliberately puts
       in an initial space in filenames to make them 'bad'.
       Note that spaces are actually valid in some pathnames
       such as in Vax logical names and NT UNC Folder names;
       the frustrum implementor is free to decide what does
       and does not constitute a valid partname or file key.

       By our convention a filename with an initial space is
       considered to be invalid. */
    if ( filename[0] == ' ' )
        {
        *ifail = FR_bad_name;
        trace_print(">>> returning from 'check_valid_filename' ifail %d\n",
                    *ifail );
        return;
        }
    /* check that all the characters are ok */
    for ( i = 0; i < len; i++ )
      {
        /* This code used to ban spaces, however, frustrum tests
           3, 5 and 10 fail if the schema directory is in a pathname
           that contains spaces. Hence this code has been changed
           to allow for this, enabling the frustrum tests to pass.
        */
        char c = filename[i];
        if ( !isprint(c) )  /* Spaces are valid in filenames */
            {
            *ifail = FR_bad_name;
            trace_print(">>> returning from 'check_valid_filename' ifail %d\n",
                        *ifail );
            return;
            }
      }

    *ifail = FR_no_errors;
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: extend_schema_filename

History:

  Jan 1991 - added
  Mar 1992 - allow schema files to be in P_SCHEMA directory
  Aug 1999 - allow P_SCHEMA to have spaces
  Oct 2014 - use new string handling

Description:

  machine specific:

  On VMS, prepend the logical symbol "P_SCHEMA:" to the named directory.

  On other platforms, evaluate the environment variable "P_SCHEMA" and
  (where set), prepend its value (and a directory separator) to the
  supplied pathname.
  If the resulting pathname would exceed max_namelen characters, the
  supplied pathname is not modified.
  check_valid_filename() is also called to check that the given filename
  is valid before proceeding.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
static void extend_schema_filename( char* filename, size_t fnsiz, int* ifail )
    {
    char extended_filename[max_namelen + 1] = "";
    trace_print( ">>> extend_schema_filename receives %s\n", filename);
    check_valid_filename( filename, ifail );
    if ( *ifail != FR_no_errors ) return;

#ifdef VMS
      {
      char *p_schema_prefix = "P_SCHEMA:";
      if ( strlen(p_schema_prefix) + strlen(filename) < max_namelen )
        /* VMS fopen accepts logical names in paths, */
        /* so we do not need to decode P_SCHEMA here */
        {
        strcpy_s( extended_filename, sizeof(extended_filename),
                                                p_schema_prefix );
        strcat_s( extended_filename, sizeof(extended_filename),
                                                filename );
        strcpy_s( filename, fnsiz, extended_filename );
        }
      }
#else
      {
      /* machine specific: An environment variable, unless on PS_IOS*/
      /* The path is allowed to contain spaces due to the change in */
      /* check_valid_filename(). */
      char *p_schema_prefix = implement_getenv( "P_SCHEMA" );
      if ( p_schema_prefix != NULL
      &&  strlen(p_schema_prefix) + 1 + strlen(filename) < max_namelen )
        {
        strcpy_s( extended_filename, sizeof(extended_filename),
                                                    p_schema_prefix );
        strcat_s( extended_filename, sizeof(extended_filename),
                                                    dir_separator_s );
        strcat_s( extended_filename, sizeof(extended_filename),
                                                    filename );
        strcpy_s( filename, fnsiz, extended_filename );
        }
      }
#endif
    trace_print( ">>> extend_schema_filename returns %s\n", filename);
    }

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: write_header

History:

  May 1990 - reformatted for example frustrum code
  Jan 1995 - max_header_line increased to allow for very long lines when
                 writing   FILE=expanded_P_SCHEMA/name   into the header
  Oct 2014 - New string handling.

Description:

  Writes standard header to file. Most keyword values are written as
  "unknown". This must be changed straight away to produce meaningful
  text - in particular the frustrum name, application name, date and
  type of machine.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void write_header( file_p file_ptr, const char* pd2hdr, int pd2len,
                          int* ifail)
    {
    char buffer[max_header_line] = "";
    trace_print(">>> write_header\n" );
    /* preamble strings do not include final newline, append on write */
    write_to_file( file_ptr, g_preamble_1, 1, strlen(g_preamble_1), ifail );
    if ( *ifail != FR_no_errors ) return;
    write_to_file( file_ptr, new_line_s, 1, strlen(new_line_s), ifail );
    if ( *ifail != FR_no_errors ) return;

    write_to_file( file_ptr, g_preamble_2, 1, strlen(g_preamble_2), ifail );
    if ( *ifail != FR_no_errors ) return;
    write_to_file( file_ptr, new_line_s, 1, strlen(new_line_s), ifail );
    if ( *ifail != FR_no_errors ) return;

    write_to_file( file_ptr, g_prefix_1, 1, strlen(g_prefix_1), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - the frustrum should write the machine name */
    strcpy_s( buffer, sizeof(buffer), "MC=unknown;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - the frustrum should write the machine model number */
    strcpy_s( buffer, sizeof(buffer), "MC_MODEL=unknown;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - the frustrum should write the machine identifier */
    strcpy_s( buffer, sizeof(buffer), "MC_ID=unknown;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - the frustrum should write the operating system name */
    strcpy_s( buffer, sizeof(buffer), "OS=unknown;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - the frustrum should write the operating system version */
    strcpy_s( buffer, sizeof(buffer), "OS_RELEASE=unknown;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - this should be replaced by your company name */
    strcpy_s( buffer, sizeof(buffer), "FRU=sdl_parasolid_customer_support;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - this should be replaced by your product's name */
    strcpy_s( buffer, sizeof(buffer), "APPL=parasolid_acceptance_tests;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - this should be replaced by your company's location */
    strcpy_s( buffer, sizeof(buffer), "SITE=unknown;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - this should be replaced by runtime user's login id */
    strcpy_s( buffer, sizeof(buffer), "USER=unknown;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    strcpy_s( buffer, sizeof(buffer), "FORMAT=" );
    strcat_s( buffer, sizeof(buffer), format_string( file_ptr->format ) );
    strcat_s( buffer, sizeof(buffer), ";\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    strcpy_s( buffer, sizeof(buffer), "GUISE=" );
    strcat_s( buffer, sizeof(buffer), guise_string( file_ptr->guise ) );
    strcat_s( buffer, sizeof(buffer), ";\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    strcpy_s( buffer, sizeof(buffer), "KEY=" );
    strcat_s( buffer, sizeof(buffer), file_ptr->key );
    strcat_s( buffer, sizeof(buffer), ";\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    strcpy_s( buffer, sizeof(buffer), "FILE=" );
    strcat_s( buffer, sizeof(buffer), file_ptr->name );
    strcat_s( buffer, sizeof(buffer), ";\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* machine specific - this should be replaced by the runtime date */
    strcpy_s( buffer, sizeof(buffer), "DATE=unknown;\n" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    write_to_file( file_ptr, g_prefix_2, 1, strlen(g_prefix_2), ifail );
    if ( *ifail != FR_no_errors ) return;

    {
    int pd2_count, buffer_count;
    trace_print(">>> part2: \"%s\" len: %d\n", pd2hdr, pd2len );

    buffer_count = 0;
    for (pd2_count = 0; pd2_count < pd2len; pd2_count++ )
      {
      char c = buffer[buffer_count] = pd2hdr[pd2_count];
      if ( c == ';' )
        {
        buffer[ buffer_count +1 ] = new_line_c;
        buffer[ buffer_count +2 ] = end_of_string_c;
        write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
        if ( *ifail != FR_no_errors ) return;
        buffer_count = 0;
        }
      else
        {
        buffer_count++;
        }
      }
    }
    write_to_file( file_ptr, g_prefix_3, 1, strlen(g_prefix_3), ifail );
    if ( *ifail != FR_no_errors ) return;

    /* trailer string does not include final newline, append on write */
    write_to_file( file_ptr, g_trailer, 1, strlen(g_trailer), ifail );
    if ( *ifail != FR_no_errors ) return;
    write_to_file( file_ptr, new_line_s, 1, strlen(new_line_s), ifail );
    if ( *ifail != FR_no_errors ) return;

    *ifail = FR_no_errors;
    }

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: write_xml_header

History:

  November 2001 - New
  October  2014 - New string handling

Description:

  Writes XML compliant header to file. This is used by the debug report
  functionality.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void write_xml_header( file_p file_ptr, const char* pd2hdr, int pd2len,
                              int* ifail)
    {
    char buffer[max_header_line] = "";
    /* <?xml version */
    strcpy_s( buffer, sizeof(buffer), "<?xml version=\"1.0\" ?>" );
    write_to_file( file_ptr, buffer, 1, strlen( buffer ), ifail );
    if ( *ifail != FR_no_errors ) return;

    *ifail = FR_no_errors;
    }

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: setup_header

History:

  May 1990 - reformatted for example frustrum code
  Oct 2014 - New string handling

Description:

  Initialise the global variables storing the text for the standard
  headers written to files.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void setup_header( void )
    {

    strcpy_s( g_preamble_1, sizeof(g_preamble_1),
              "**" );                           /* two asterisks */
    strcat_s( g_preamble_1, sizeof(g_preamble_1),
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ" );   /* upper case letters */
    strcat_s( g_preamble_1, sizeof(g_preamble_1),
              "abcdefghijklmnopqrstuvwxyz" );   /* lower case letters */
    strcat_s( g_preamble_1, sizeof(g_preamble_1),
              "**************************" );   /* twenty six asterisks */


    strcpy_s( g_preamble_2, sizeof(g_preamble_2),
              "**" );                           /* two asterisks */
    strcat_s( g_preamble_2, sizeof(g_preamble_2),
              "PARASOLID" );                    /* PARASOLID (upper case) */
    strcat_s( g_preamble_2, sizeof(g_preamble_2),
              " !" );                           /* space and exclamation */
    strcat_s( g_preamble_2, sizeof(g_preamble_2),
              "\"" );                           /* a double quote char */
    strcat_s( g_preamble_2, sizeof(g_preamble_2),
              "#$%&'()*+,-./:;<=>?@[" );        /* some special chars */
    strcat_s( g_preamble_2, sizeof(g_preamble_2),
              "\\" );                           /* a backslash char */
    strcat_s( g_preamble_2, sizeof(g_preamble_2),
              "]^_`{|}~" );                     /* more special chars */
    strcat_s( g_preamble_2, sizeof(g_preamble_2),
              "0123456789" );                   /* digits */
    strcat_s( g_preamble_2, sizeof(g_preamble_2),
              "**************************" );   /* twenty six asterisks */

    strcpy_s( g_trailer, sizeof(g_trailer), g_trailer_start );
    strcat_s( g_trailer, sizeof(g_trailer),
        "*****************************************************************" );

    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: skip_header

History:

  May 1990 - reformatted for example frustrum code
  Oct 2014 - New string handling

Description:

  Skip header information when opening a file for read.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void skip_header( file_p file_ptr, int* ifail )
    {
    char buffer[max_header_line] = "";
    int chars_read = 0;
    int end_header = 0;
    int first_line = 1;
    trace_print(">>> skip_header " );
    while (!end_header)
        {
        /* read from the file */
        read_from_file( file_ptr, buffer, 1, max_header_line, &chars_read, ifail );
        if ( *ifail != FR_no_errors ) return;
        /***
        trace_print(">>> buffer:   %s", buffer );
        ***/

        if (strncmp( buffer, g_trailer_start, strlen( g_trailer_start )) == 0)
            {
            /*** this is the end of the header */
            end_header = 1;
            }
        else
        if (first_line
        &&  strncmp( buffer, g_preamble_1, strlen( g_preamble_1 ) ) != 0)
            {
            /*
            rewind the file to the beginning as the header is not there
            (this must be a Parasolid version 1 or Romulus version 6 file
            */
            trace_print(">>> rewinding the file");
            rewind( file_ptr->stream );
            end_header = 1;
            }
        else
            {
            /*  line skipped  */
            }

        first_line = 0;
        }
    *ifail = FR_no_errors;
    }



/*=============================================================================
                 EXTERNAL ROUTINES
=============================================================================*/



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FSTART

History:

  May 1990 - reformatted for example frustrum code

Description:

  Start frustrum; set up file structures if not already done

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FSTART( int* ifail )
    {
    int i;
    *ifail = FR_unspecified;
    trace_print(">>> FSTART\n");

    if (frustrum_started == 0)
    {
    for ( i = 0; i < max_open_files; i++ )
      stream_id[i] = 0;

    /* set up the global variables required for writing
    frustrum file headers */
    setup_header();

#ifdef PS_AIX
    /* associate function "danger_catcher" with the condition SIGDANGER */
    signal(SIGDANGER, danger_catcher);
    short_of_memory = 0; /* set the global flag as false */
#endif
    }

    frustrum_started++;

    *ifail = FR_no_errors;
    trace_print(">>> returning from FSTART with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FSTOP

History:

  May 1990 - reformatted for example frustrum code
  Nov 1992 - reset input_output_buffer to 0 on free

Description:

  Stop frustrum. Does nothing much.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FSTOP( int* ifail )
    {
    *ifail = FR_unspecified;
    trace_print(">>> FSTOP\n");

    if (frustrum_started <= 0)
        {
        *ifail = FR_not_started;
        trace_print(">>> returning from FSTOP with ifail %d\n", *ifail );
        return;
        }

    frustrum_started--;

    if ( input_output_buffer != NULL )
        {
        input_output_buflen = 0;
        free(input_output_buffer);
        input_output_buffer = NULL;
        }

    if ( frustrum_started == 0 )
      {
      file_p file_ptr = open_files;

      /* while there are still files open - close them down */
      while (file_ptr != NULL)
        {
        fclose( file_ptr->stream );
        if (file_ptr->next == NULL)
          {
          /* free the space used in the file pointer */
          free( file_ptr );
          file_ptr = NULL;
          }
        else
          {
          file_ptr = file_ptr->next;
          free( file_ptr->prev );
          }
        }

      /* reset variables and return values */
      file_count = 0;
      open_files = NULL;
      }

    *ifail = FR_no_errors;
    trace_print(">>> returning from FSTOP with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FMALLO

History:

  May 1990 - reformatted for example frustrum code

Description:

  Attempts to allocate memory as requested

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FMALLO( int* nbytes, char** memory, int* ifail)
    {
    *ifail = FR_unspecified;
    trace_print(">>> FMALLO %d\n", *nbytes);
    if (frustrum_started <= 0)
        {
        *memory = 0;

        *ifail = FR_not_started;
        trace_print(">>> returning from FMALLO with ifail %d\n", *ifail );
        return;

        }

#ifdef PS_AIX
    if (short_of_memory) /* operating system has warned danger_catcher of it */
        {
        /* when the Frustrum tells Parasolid it has run out of virtual memory,
           Parasolid will perform some housekeeping tasks which will free up
           some space, hence it is appropriate to reset the global flag here */
        *memory = NULL;
        short_of_memory = 0;
        }
    else if (psdanger(SIGKILL)*PAGESIZE <= *nbytes)
        {
        /* psdanger(SIGKILL) returns the current number of free paging
           space blocks minus the op system paging space kill threshold */
        *memory = NULL;
        }
#endif

    /* Enforce memory limit */
    if( frustrum_memory_allocated + (*nbytes) > MAX_MEMORY_ALLOCATION)
        *memory = NULL;
    else
        *memory = (char *) malloc( *nbytes );

    if (*memory == NULL)
        {
        *ifail = FR_memory_full;
        trace_print(">>> returning from FMALLO with ifail %d\n", *ifail );
        return;
        }

    /* account for memory */
    frustrum_memory_allocated += (*nbytes);

    *ifail = FR_no_errors;
    trace_print(">>> returning from FMALLO with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FMFREE

History:

  May 1990 - reformatted for example frustrum code

Description:

  Frees memory

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FMFREE( int* nbytes, char** memory, int* ifail )
    {
    *ifail = FR_unspecified;
    trace_print(">>> FMFREE\n");
    if (frustrum_started <= 0)
        {
        *ifail = FR_not_started;
        trace_print(">>> returning from FMFREE with ifail %d\n", *ifail );
        return;
        }

    free( *memory );
    frustrum_memory_allocated -= (*nbytes);    /* account for it */

    *ifail = FR_no_errors;
    trace_print(">>> returning from FMFREE with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FFOPRD

History:

  May 1990 - reformatted for example frustrum code
  Oct 2014 - New string handling

Description:

  Opens a file for read. A file extension is added to show the guise
  and format of the file. If requested, all the line containing the file
  header will be skipped.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FFOPRD( const int* guise, const int* format, const char* name,
                    const int* namlen, const int* skiphd, int* strid,
                    int* ifail )
    {
    char   keyname[max_namelen+1] = "";  /* holds key + null char     */
    char  filename[max_namelen+1] = "";  /* holds key + extension     */
    FILE *stream;
    file_p file_ptr;
    int filesystem = PS_LONG_NAME;

    *ifail = FR_unspecified;
    *strid = null_strid;

    trace_print(">>> FFOPRD %d %d %d\n", *guise, *format, *skiphd);
    if (frustrum_started <= 0)
      {
      *ifail = FR_not_started;
      trace_print(">>> returning from FFOPRD with ifail %d\n", *ifail );
      return;
      }

    /* check that limit has not been reached */
    if (file_count == max_open_files)
      {
      *ifail = FR_open_fail;
      trace_print(">>> returning from FFOPRD with ifail %d\n", *ifail );
      return;
      }

    memcpy_s( keyname, sizeof(keyname), name, *namlen );
    keyname[*namlen]  = end_of_string_c;

    memcpy_s( filename, sizeof(filename), name, *namlen );
    filename[*namlen] = end_of_string_c;

    if ( *guise == FFCSCH )
        {
#ifndef _WIN32
        /* force schema basename to lowercase for consistency */
        /* (the case of all NT filenames are converted later) */
        int i;
        for ( i = 0 ; i < *namlen ; i++ )
            filename[i] = tolower(filename[i]);
#endif
        /* add (and decode) a P_SCHEMA prefix to the filename */
        extend_schema_filename(filename, sizeof(filename), ifail);
        if ( *ifail != FR_no_errors )
           {
           trace_print(">>> returning from FFOPRD with ifail %d\n", *ifail );
           return;
           }
        }

#ifdef _WIN32
    /* check whether file will be read from short filename system  */
    filesystem = nt_filesystem_type(filename);
    {
    int i = 0;
    int start = 0;
    /* locate the filename portion in the pathname */
    do
        {
        if ( filename[i] == dir_separator_c )
            start = i+1;
        i++;
        }
    while ( filename[i] != '\0' );

    /* force the filename part of short (DOS) filenames to be uppercase */
    /*                         and that of other filenames to lowercase */
    /* the case is not significant on NT filenames but it is preserved. */
    for ( i = start ; filename[i] != '\0' ; i++ )
      {
      if ( filesystem == PS_SHORT_NAME )
        filename[i] = toupper(filename[i]);
      else
        filename[i] = tolower(filename[i]);
      }
    }
#endif

    {
    /* add the file extension */
    char *gui  = filetype_guise_string( *guise, filesystem );
    strcat_s( filename, sizeof(filename), gui );
    if( *guise != FFCXMO )
      {
      char *fmt = filetype_format_string( *format, filesystem );
      strcat_s( filename, sizeof(filename), fmt );
      }
    }

    trace_print(">>> filename \"%s\"\n", filename );

    check_valid_filename( filename, ifail );
    if ( *ifail != FR_no_errors )
      {
      trace_print(">>> returning from FFOPRD with ifail %d\n", *ifail );
      return;
      }

    /* open file for reading */
#ifdef _WIN32
    if (*format == FFBNRY)
      /* if binary file is opened with "r" instead of "rb" reading will fail */
      /* with end-of-file error, if it reads byte with value equal to CTRL-Z */
        stream = implement_fopen( filename, g_fopen_mode_read_binary );
    else
        stream = implement_fopen( filename, g_fopen_mode_read_text );
#else
    stream = implement_fopen( filename, g_fopen_mode_read );
#endif

    if (stream == 0)
      {
      *ifail = FR_not_found;
      trace_print(">>> returning from FFOPRD with ifail %d\n", *ifail );
      return;
      }

    new_open_file( stream, *guise, *format, read_access,
                   filename, keyname, &file_ptr, ifail );
    if ( *ifail != FR_no_errors )
      {
      trace_print(">>> returning from FFOPRD with ifail %d\n", *ifail );
      return;
      }


    if (*skiphd == FFSKHD)
      {
      skip_header( file_ptr, ifail );
      if ( *ifail != FR_no_errors )
        {
        trace_print(">>> returning from FFOPRD with ifail %d\n", *ifail );
        return;
        }
      }

    *strid = file_ptr->strid;
    trace_print(">>> strid %d\n", *strid );

    *ifail = FR_no_errors;
    trace_print(">>> returning from FFOPRD with ifail %d\n", *ifail );
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FFOPWR

History:

  May 1990      - reformatted for example frustrum code
  November 2001 - added support for debug report files
  October 2014  - New string handling

Description:

  Opens file to be written and writes to it the standard file header.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FFOPWR( const int* guise, const int* format, const char* name,
                    const int* namlen, const char* pd2hdr, const int *pd2len,
                    int *strid, int *ifail )
    {
    char   keyname[max_namelen+1] = "";  /* holds key + null char    */
    char  filename[max_namelen+1] = "";  /* holds key + extension    */
    FILE *stream;
    file_p file_ptr;
    int filesystem = PS_LONG_NAME;

    *ifail = FR_unspecified;
    *strid = null_strid;
    trace_print(">>> FFOPWR %d %d\n", *guise, *format );

    if (frustrum_started <= 0)
      {
      *ifail = FR_not_started;
      trace_print(">>> returning from FFOPWR with ifail %d\n", *ifail );
      return;
      }


    if (file_count == max_open_files)
      {
      *ifail = FR_open_fail;
      trace_print(">>> returning from FFOPWR with ifail %d\n", *ifail );
      return;
      }

    memcpy_s(  keyname, sizeof(keyname),  name, *namlen );
    keyname[*namlen]  = end_of_string_c;

    memcpy_s( filename, sizeof(filename), name, *namlen );
    filename[*namlen] = end_of_string_c;

    if ( *guise == FFCSCH )
        {
#ifndef _WIN32
        /* force schema basename to lowercase for consistency  */
        /* (the case of NT all filenames are converted later) */
        int i;
        for ( i = 0 ; i < *namlen ; i++ )
            filename[i] = tolower(filename[i]);
#endif
        /* add (and decode) a P_SCHEMA prefix to the filename  */
        extend_schema_filename(filename, sizeof(filename), ifail);
        if ( *ifail != FR_no_errors )
           {
           trace_print(">>> returning from FFOPWR with ifail %d\n", *ifail );
           return;
           }
        }

#ifdef _WIN32
    /* check whether file will be read from a short filename system  */
    filesystem = nt_filesystem_type(filename);
    {
    int i = 0;
    int start = 0;
    /* locate the filename portion in the pathname */
    do
        {
        if ( filename[i] == dir_separator_c )
            start = i+1;
        i++;
        }
    while ( filename[i] != '\0' );

    /* force the filename part of short (DOS) filenames to be uppercase */
    /*                         and that of other filenames to lowercase */
    /*  the case is not significant on NT filenames but it is preserved */
    for ( i = start ; filename[i] != '\0' ; i++ )
      {
      if ( filesystem == PS_SHORT_NAME )
        filename[i] = toupper(filename[i]);
      else
        filename[i] = tolower(filename[i]);
      }
    }
#endif

    {
    /* add the file extension */
    char *gui  = filetype_guise_string( *guise, filesystem );
    strcat_s( filename, sizeof(filename), gui );
    if( *guise != FFCXMO && *guise != FFCDBG )
      {
      char *fmt = filetype_format_string( *format, filesystem );
      strcat_s( filename, sizeof(filename), fmt );
      }
    }

    trace_print(">>> filename \"%s\"\n", filename );

    check_valid_filename( filename, ifail );
    if ( *ifail != FR_no_errors )
      {
      trace_print(">>> returning from FFOPWR with ifail %d\n", *ifail );
      return;
      }

    /* open file for writing */
    stream = implement_fopen( filename, g_fopen_mode_write );
    if (stream == 0)
      {
      *ifail = FR_already_exists;
      trace_print(">>> returning from FFOPWR with ifail %d\n", *ifail );
      return;
      }

    new_open_file( stream, *guise, *format, write_access,
                  filename, keyname, &file_ptr, ifail );
    if ( *ifail != FR_no_errors )
      {
      trace_print(">>> returning from FFOPWR with ifail %d\n", *ifail );
      return;
      }

    if ( *guise == FFCDBG )
        write_xml_header( file_ptr, pd2hdr, *pd2len, ifail );
    else
        write_header( file_ptr, pd2hdr, *pd2len, ifail );

    if ( *ifail != FR_no_errors )
      {
      trace_print(">>> returning from FFOPWR with ifail %d\n", *ifail );
      return;
      }

    *strid = file_ptr->strid;
    trace_print(">>> strid %d\n", *strid );
    *ifail = FR_no_errors;
    trace_print(">>> returning from FFOPWR with ifail %d\n", *ifail );
    }


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FFOPRB

History:

  May 1990 - reformatted for example frustrum code
  Oct 2014 - New string handling

Description:

  Opens temporary rollback file for read/write.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FFOPRB( const int* guise, const int* minsiz, const int* maxsiz,
                    int* actsiz, int* strid, int* ifail )
    {
    char filename[max_namelen+1] = "";
    char  keyname[max_namelen+1] = "";
    FILE *stream;
    file_p file_ptr;
    *ifail = FR_unspecified;
    *strid = null_strid;
    trace_print(">>> FFOPRB\n");
    if (frustrum_started <= 0)
      {
      *ifail = FR_not_started;
      trace_print(">>> returning from FFOPRB with ifail %d\n", *ifail );
      return;
      }

    if (file_count == max_open_files)
      {
      *ifail = FR_open_fail;
      trace_print(">>> returning from FFOPRB with ifail %d\n", *ifail );
      return;
      }

    if (*guise != FFCROL)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFOPRB with ifail %d\n", *ifail );
      return;
      }


    strcpy_s( filename, sizeof(filename), "rollback.001" );
    strcpy_s( keyname , sizeof(keyname),  "rollback"     );


    /* open file */
    stream = implement_fopen( filename, g_fopen_mode_append );
    if (stream == 0)
      {
      *ifail = FR_open_fail;
      trace_print(">>> returning from FFOPRB with ifail %d\n", *ifail );
      return;
      }

    new_open_file( stream, *guise, FFBNRY, read_write_access,
                          filename, keyname, &file_ptr, ifail );
    if ( *ifail != FR_no_errors )
      {
      trace_print(">>> returning from FFOPRB with ifail %d\n", *ifail );
      return;
      }

    *actsiz = *maxsiz;
    *strid = file_ptr->strid;
    trace_print(">>> strid %d\n", *strid );

    *ifail = FR_no_errors;
    trace_print(">>> returning from FFOPRB with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FFWRIT

History:

  May 1990      - reformatted for example frustrum code
  November 2001 - flush debug report and journal files after write

Description:

  Write buffer to open file.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FFWRIT( const int* guise, const int* strid, const int* nchars,
                    const char* buffer, int* ifail)
    {
    file_p file_ptr;
    *ifail = FR_unspecified;
    trace_print(">>> FFWRIT %d %d %d\n", *guise, *strid, *nchars);

    if (frustrum_started <= 0)
      {
      *ifail = FR_not_started;
      trace_print(">>> returning from FFWRIT with ifail %d\n", *ifail );
      return;
      }

    /* find the file info for this stream-id  */
    for ( file_ptr = open_files; file_ptr != NULL; file_ptr = file_ptr->next )
      {
      if (file_ptr->strid == *strid) break;
      }

    if (file_ptr == NULL)
      {
      *ifail = FR_internal_error;
      trace_print(">>> returning from FFWRIT with ifail %d\n", *ifail );
      return;
      }


    /* check file guise */
    if (*guise != file_ptr->guise)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFWRIT with ifail %d\n", *ifail );
      return;
      }

    /* check access */
    if (file_ptr->access != write_access &&
        file_ptr->access != read_write_access)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFWRIT with ifail %d\n", *ifail );
      return;
      }


    write_to_file( file_ptr, buffer, 0, *nchars, ifail );
    if ( *ifail != FR_no_errors )
      {
      trace_print(">>> returning from FFWRIT with ifail %d\n", *ifail );
      return;
      }

    /* If we are writing a journal or debug report file then flush the */
    /* buffer - this to ensure that in the event of crash as much data */
    /* is preserved as possible */
    if (*guise == FFCJNL || *guise == FFCDBG)
           fflush( file_ptr->stream );

    *ifail = FR_no_errors;
    trace_print(">>> returning from FFWRIT with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FFREAD

History:

  May 1990 - reformatted for example frustrum code

Description:

  Read buffer from open file.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FFREAD( const int* guise, const int* strid, const int* nmax,
                    char* buffer, int* nactual, int* ifail)
    {
    file_p file_ptr;
    int chars_read = 0;
    *ifail = FR_unspecified;
    *nactual = 0;
    trace_print(">>> FFREAD: %d %d %d\n", *guise, *strid, *nmax);

    /* check that the frustrum has been started */
    if (frustrum_started <= 0)
      {
      *ifail = FR_not_started;
      trace_print(">>> returning from FFREAD with ifail %d\n", *ifail );
      return;
      }


    /* find the correct file pointer */
    for ( file_ptr = open_files; file_ptr != NULL; file_ptr = file_ptr->next )
      {
      if (file_ptr->strid == *strid) break;
      }
    if (file_ptr == NULL)
        {
        *ifail = FR_internal_error;
        trace_print(">>> returning from FFREAD with ifail %d\n", *ifail );
        return;
        }


    /* check file guise */
    if (*guise != file_ptr->guise)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFREAD with ifail %d\n", *ifail );
      return;
      }

    /* check access */
    if (file_ptr->access != read_access &&
        file_ptr->access != read_write_access)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFREAD with ifail %d\n", *ifail );
      return;
      }

    /* read the information from the file */
    read_from_file( file_ptr, buffer, 0, *nmax, &chars_read, ifail );
    if ( *ifail != FR_no_errors )
      {
      trace_print(">>> returning from FFREAD with ifail %d\n", *ifail );
      return;
      }

    /***
    {
    int count;
    for( count = 0; count < chars_read; count ++ )
      {
      trace_print(">>> FFREAD - buffer[%d]: %d  \"%c\"\n",
          count, buffer[count], buffer[count] );
      }
    }
    ***/


    *nactual = chars_read;
    trace_print(">>> FFREAD: %d bytes read\n", *nactual );
    *ifail = FR_no_errors;
    trace_print(">>> returning from FFREAD with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FFTELL

History:

  May 1990 - reformatted for example frustrum code

Description:

  Indicate position in rollback file.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FFTELL( const int* guise, const int* strid, int* pos, int* ifail )
    {
    file_p file_ptr;
    *ifail = FR_unspecified;
    trace_print(">>> FFTELL %d %d\n", *guise, *strid);

    if (frustrum_started <= 0)
        {
        *ifail = FR_not_started;
        trace_print(">>> returning from FFTELL with ifail %d\n", *ifail );
        return;
        }


    if (*guise != FFCROL)
        {
        *ifail = FR_unspecified;
        trace_print(">>> returning from FFTELL with ifail %d\n", *ifail );
        return;
        }

    /* check file is open */
    for ( file_ptr = open_files; file_ptr != NULL; file_ptr = file_ptr->next )
      {
        if (file_ptr->strid == *strid) break;
      }

    if (file_ptr == NULL)
        {
        *ifail = FR_internal_error;
        trace_print(">>> returning from FFTELL with ifail %d\n", *ifail );
        return;
        }


    /* check file guise */
    if (*guise != file_ptr->guise)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFTELL with ifail %d\n", *ifail );
      return;
      }

    /* note file pointer */
    *pos = ftell( file_ptr->stream );

    *ifail = FR_no_errors;
    trace_print(">>> returning from FFTELL with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FFSEEK

History:

  May 1990 - reformatted for example frustrum code

Description:

  Change position in rollback file.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FFSEEK( const int* guise, const int* strid, const int* pos,
                    int* ifail )
    {
    file_p file_ptr;
    *ifail = FR_unspecified;
    trace_print(">>> FFSEEK %d %d\n", *guise, *strid);

    if (frustrum_started <= 0)
      {
      *ifail = FR_not_started;
      trace_print(">>> returning from FFSEEK with ifail %d\n", *ifail );
      return;
      }

    if (*guise != FFCROL)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFSEEK with ifail %d\n", *ifail );
      return;
      }

    /* check file is open */
    for ( file_ptr = open_files; file_ptr != NULL; file_ptr = file_ptr->next )
      {
      if (file_ptr->strid == *strid) break;
      }

    if (file_ptr == NULL)
      {
      *ifail = FR_internal_error;
      trace_print(">>> returning from FFSEEK with ifail %d\n", *ifail );
      return;
      }


    /* check file guise */
    if (*guise != file_ptr->guise)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFSEEK with ifail %d\n", *ifail );
      return;
      }

    /* reset file pointer */
    if (fseek( file_ptr->stream, (long) (*pos), 0 ) != 0)
      {
      *ifail = FR_unspecified;
      trace_print(">>> returning from FFSEEK with ifail %d\n", *ifail );
      return;
      }


    *ifail = FR_no_errors;
    trace_print(">>> returning from FFSEEK with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FFCLOS

History:

  May 1990 - reformatted for example frustrum code
  Oct 2014 - New string handling

Description:

  Close specified file. If a rollback file or the action is abort then
  delete the file.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FFCLOS( const int* guise, const int* strid, const int* action,
                    int* ifail )
    {
    file_p file_ptr;
    char filename[max_namelen+1] = "";
    int delete_it = 0;
    *ifail = FR_unspecified;
    trace_print(">>> FFCLOS %d %d\n", *guise, *strid);

    if (frustrum_started <= 0)
      {
      *ifail = FR_not_started;
      trace_print(">>> returning from FFCLOS with ifail %d\n", *ifail );
      return;
      }


    /* find the file info for this stream-id  */
    for ( file_ptr = open_files; file_ptr != NULL; file_ptr = file_ptr->next )
      {
      if (file_ptr->strid == *strid) break;
      }

    if (file_ptr == NULL)
      {
      *ifail = FR_close_fail;
      trace_print(">>> returning from FFCLOS with ifail %d\n", *ifail );
      return;
      }


    if ( file_ptr->access == read_write_access ||
         (file_ptr->access == write_access && *action == FFABOR) )
      {
      delete_it = 1;
      strcpy_s( filename, sizeof(filename), file_ptr->name );
      }


    /* close file */
    stream_id[file_ptr->strid - 1] = 0;
    if (fclose( file_ptr->stream ) == EOF)
      {
      *ifail = FR_close_fail;
      trace_print(">>> returning from FFCLOS with ifail %d\n", *ifail );
      return;
      }

    if (file_ptr == open_files)
        open_files = open_files->next;
    else
        file_ptr->prev->next = file_ptr->next;

    if (file_ptr->next != NULL)
        file_ptr->next->prev = file_ptr->prev;

    free( file_ptr );
    file_count--;

    if (delete_it)
      {
      delete_file( filename, ifail );
      if ( *ifail != FR_no_errors )
        {
        trace_print(">>> returning from FFCLOS with ifail %d\n", *ifail );
        return;
        }
      }

    *ifail = FR_no_errors;
    trace_print(">>> returning from FFCLOS with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FABORT

History:

  May 1990 - reformatted for example frustrum code
  Oct 1992 - activity moved to FSTOP

Description:

  Aborting a kernel operation. In this implementation, it does nothing

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FABORT( int* ifail )
    {
    *ifail = FR_no_errors;
    trace_print(">>> FABORT\n");
    trace_print(">>> returning from FABORT with ifail %d\n", *ifail );
    }



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: FTMKEY

History:

  May 1990 - reformatted for example frustrum code
  Aug 1999 - Make invalid filenames start with a space.
           - Also initialise filename variable here (and all chars
             elsewhere) to cure compile problems on NT VC6.
  Oct 2014 - New string handling

Description:

  Returns sample valid or invalid file key depending on whether
  the given index is positive/zero(valid) or negative (invalid).

  The name generated on NT is only 8 characters long
  because this function can't tell whether this will
  eventually be used to name a short DOS or NTFS file.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void FTMKEY( int* guise, int* format, int* index, char name[],
                    int* namlen, int* ifail )
    {
    char filename[max_namelen+1] = "";
    char *temp;
    *ifail = FR_unspecified;
    trace_print(">>> FTMKEY %d %d %d\n",*guise,*format,*index);

    /* The TESTFR frustrum tests ask FTMKEY to generate valid and invalid */
    /* sample filenames, so it can simulate the effect of an application  */
    /* passing an invalid key to GETMOD, GETSNP etc. which is then handed */
    /* down to the application frustrum.                                  */
    /* If TESTFR calls FTMKEY with a negative valued index it wants it to */
    /* construct a test filename which FFOPRD/FFOPWR will later reject    */

    /* machine specific: if index < 0, generate an invalid filename */
    /*                     which check_valid_filename() will detect */
    /*                     else generate a filename which is valid  */

    /* By our convention an initial space indicates an invalid filename */
    if (*index < 0)
      strcpy_s( filename, sizeof(filename), " " );
    else
      /* skip */;

#ifdef _WIN32
    /* generate short names suitable for use on FAT file systems */
    strcat_s( filename, sizeof(filename), "D" );
    temp = filetype_guise_string( *guise, PS_SHORT_NAME );
    strcat_s( filename, sizeof(filename), &temp[1] );
                                        /* skip . at start of filetype */
    /* add string to indicate the format used */
    if (*guise == FFCXMO)
        strcat_s( filename, sizeof(filename), "_o" );
    else
      {
      char *fmt = filetype_format_string( *format, PS_SHORT_NAME );
      strcat_s( filename, sizeof(filename), fmt );
      }
#else
    /* generate self explanatory filenames */
    strcat_s( filename, sizeof(filename), "dummy_" );
    temp = filetype_guise_string( *guise, PS_LONG_NAME );
    strcat_s( filename, sizeof(filename), &temp[1] );
                                    /* skip . at start of filetype */
    /* add string to indicate the format used */
    if (*guise == FFCXMO)
        strcat_s( filename, sizeof(filename), "_o" );
    else
      {
      char *fmt = filetype_format_string( *format, PS_LONG_NAME );
      strcat_s( filename, sizeof(filename), fmt );
      }
#endif

    /* add an identifying value */
    if ( abs(*index) <= 20 )
      {
      int i = 0;
      int idx = abs(*index);
      char num[4] = "";
      num[i++] = '_';
      num[i++] = '0' + idx / 10;
      num[i++] = '0' + idx % 10;
      num[i++] = '\0';
      strcat_s( filename, sizeof(filename), num );
      }
    else
      strcat_s( filename, sizeof(filename), "___" );

    /* for a valid filename add character(s) to the filename indicating */
    /* filename is supposed to be valid                                 */

#ifdef _WIN32
    if (*index >= 0)
      strcat_s( filename, sizeof(filename), "V" );
    else
      /* skip */;
#else
    if (*index >= 0)
      strcat_s( filename, sizeof(filename), "_valid" );
    else
      /* skip */;
#endif

    /* ensure that the length of filename is acceptable */
    *namlen = (int) strlen( filename );
    if (*namlen > max_namelen)
      {
      *ifail = FR_internal_error;
      trace_print(">>> returning from FTMKEY with ifail %d\n", *ifail );
      return;
      }


    strcpy_s( name, max_namelen+1, filename );
    trace_print(">>> filename \"%s\" %d\n", name, *namlen );

    *ifail = FR_no_errors;
    trace_print(">>> returning from FTMKEY with ifail %d\n", *ifail );
    }



/*=============================================================================
                 GO ROUTINES
=============================================================================*/



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: GOOPPX

History:

  May 1990 - reformatted for example frustrum code
  Oct 1994 - return CONTIN

Description:

  Open Pixel Data - dummy routine - return code for 'continue, no errors'.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void GOOPPX( const int* nreals, const double* rvals, const int* nints,
                    const int* ivals, int* ifail)
{
    trace_print(">>> GOOPPX\n" );
    *ifail = CONTIN;
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: GOCLPX

History:

  May 1990 - reformatted for example frustrum code
  Oct 1994 - return CONTIN

Description:

  Open Pixel Data - dummy routine - return code for 'continue, no errors'.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void GOCLPX( const int* nreals, const double* rvals, const int* nints,
                    const int* ivals, int* ifail)
{
    trace_print(">>> GOCLPX\n" );
    *ifail = CONTIN;
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: GOPIXL

History:

  May 1990 - reformatted for example frustrum code
  Oct 1994 - return CONTIN

Description:

  Open Pixel Data - dummy routine - return code for 'continue, no errors'.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void GOPIXL( const int* nreals, const double* rvals, const int* nints,
                    const int* ivals, int* ifail)
{
    trace_print(">>> GOPIXL\n" );
    *ifail = CONTIN;
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: GOOPSG

History:

  May 1990 - reformatted for example frustrum code
  Oct 1994 - return CONTIN

Description:

  Open Pixel Data - dummy routine - return code for 'continue, no errors'.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void GOOPSG( const int* segtyp, const int* ntags, const int* tags,
                    const int* ngeom, const double* geom, const int* nlntp,
                    const int* lntp, int* ifail)
{
    trace_print(">>> GOOPSG\n" );
    *ifail = CONTIN;
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: GOCLSG

History:

  May 1990 - reformatted for example frustrum code
  Oct 1994 - return CONTIN

Description:

  Open Pixel Data - dummy routine - return code for 'continue, no errors'.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void GOCLSG( const int* segtyp, const int* ntags, const int* tags,
                    const int* ngeom, const double* geom, const int* nlntp,
                    const int* lntp, int* ifail)
{
    trace_print(">>> GOCLSG\n" );
    *ifail = CONTIN;
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Function: GOSGMT

History:

  May 1990 - reformatted for example frustrum code
  Oct 1994 - return CONTIN

Description:

  Open Pixel Data - dummy routine - return code for 'continue, no errors'.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern void GOSGMT( const int* segtyp, const int* ntags, const int* tags,
                    const int* ngeom, const double* geom, const int* nlntp,
                    const int* lntp, int* ifail)
{
    trace_print(">>> GOSGMT\n" );
    *ifail = CONTIN;
}

