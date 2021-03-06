#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sgtty.h>
#include <signal.h>
#include <pwd.h>

#include "sys5.h"

#ifdef TMC
#include <ctools.h>
#else
#include "ctools.h"
#endif
#include "menu.h"
#include "mem.h"

#include "rolofiles.h"
#include "rolodefs.h"
#include "datadef.h"

/**
 * NOTE: Mine
 */
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "rolo.h"
#include "clear.h"
#include "io.h"
#include "rlist.h"
#include "options.h"

static char rolodir[DIRPATHLEN]; /* directory where rolo data is */
static char filebuf[DIRPATHLEN]; /* stores result of homedir() */

int changed = 0;
int reorder_file = 0;
int rololocked = 0;
int in_search_mode = 0;

char *rolo_emalloc(int size)
{
    // if (size <= 0) {
    //     save_and_exit(-1);
    // }
    // if (size <= 0) {
    //     fprintf(stderr, "Fatal error:  invalid malloc amt\n");
    //     save_and_exit(-1);
    // }
    /* error handling memory allocator */
    char *rval = malloc(size);
    if (rval == 0)
    {
        fprintf(stderr, "Fatal error:  out of memory\n");
        save_and_exit(-1);
    }

    ALLOCATED_MEM[ALLOCATED_INDEX] = rval;
    ALLOCATED_INDEX++;

    return (rval);
}

char *copystr(s) char *s;

/* memory allocating string copy routine */

{
    if (s == 0)
        return (0);

    char *copy = rolo_emalloc(strlen(s) + 1);
    strcpy(copy, s);
    return (copy);
}

char *timestring()

/* returns a string timestamp */

{
    char *s;
    long timeval;
    time(&timeval);
    s = ctime(&timeval);
    s[strlen(s) - 1] = '\0';
    return (copystr(s));
}

void user_interrupt()
{
    /* if the user hits C-C (we assume he does it deliberately) */

    unlink(homedir(ROLOLOCK));
    fprintf(stderr, "\nAborting rolodex, no changes since last save recorded\n");
    exit(-1);
}

void user_eof()
{
    /* if the user hits C-D */

    unlink(homedir(ROLOLOCK));
    fprintf(stderr, "\nUnexpected EOF on terminal. Saving rolodex and exiting\n");
    save_and_exit(-1);
}

void roloexit(int rval)
{
    if (rololocked) {
        unlink(homedir(ROLOLOCK));
    }

    if (free_io_mem() == -1) {
        fprintf(stderr, "unable to free io prog mem");
    }

    exit(rval);
}

void save_to_disk()

/* move the old rolodex to a backup, and write out the new rolodex and */
/* a copy of the new rolodex (just for safety) */

{
    FILE *tempfp, *copyfp;
    char d1[DIRPATHLEN], d2[DIRPATHLEN];

    /**
   * TODO: Commented out b/c unused
   */
    //int r;

    tempfp = fopen(homedir(ROLOTEMP), "w");
    copyfp = fopen(homedir(ROLOCOPY), "w");
    if (tempfp == NULL || copyfp == NULL)
    {
        fprintf(stderr, "Unable to write rolodex...\n");
        fprintf(stderr, "Any changes made have not been recorded\n");
        roloexit(-1);
    }
    if (write_rolo(tempfp, copyfp) == -1) {
        fprintf(stderr, "Unable to write to file, maybe out of disk space?");
        roloexit(-1);
    }
    if (fclose(tempfp) == EOF) {
        fprintf(stderr, "error closing file");
        roloexit(-1);
    }
    if (fclose(copyfp) == EOF) {
        fprintf(stderr, "error closing file");
        roloexit(-1);
    }
    if (rename(strcpy(d1, homedir(ROLODATA)), strcpy(d2, homedir(ROLOBAK))) ||
        rename(strcpy(d1, homedir(ROLOTEMP)), strcpy(d2, homedir(ROLODATA))))
    {
        fprintf(stderr, "Rename failed.  Revised rolodex is in %s\n", ROLOCOPY);
        roloexit(-1);
    }
    printf("Rolodex saved\n");
    sleep(1);
    changed = 0;
}

void save_and_exit(int rval)
{
    if (changed)
        save_to_disk();
    roloexit(rval);
}

extern struct passwd *getpwnam();

char *home_directory(name) char *name;
{
    struct passwd *pwentry;
    if (0 == (pwentry = getpwnam(name)))
        return ("");
    return (pwentry->pw_dir);
}

char *homedir(filename) char *filename;

/* e.g., given "rolodex.dat", create "/u/massar/rolodex.dat" */
/* rolodir generally the user's home directory but could be someone else's */
/* home directory if the -u option is used. */

{
    nbuffconcat(filebuf, 3, rolodir, "/", filename);
    return (filebuf);
}

char *libdir(filename) char *filename;

/* return a full pathname into the rolodex library directory */
/* the string must be copied if it is to be saved! */

{
    nbuffconcat(filebuf, 3, ROLOLIB, "/", filename);
    return (filebuf);
}

int rolo_only_to_read()
{
    return ((OPTION_SUMMARY_FLAG == 1) || NON_OPTION_ARGS > 0);
}

void locked_action()
{
    if (OPTION_OTHERUSER_FLAG == 1)
    {
        fprintf(stderr, "Someone else is modifying that rolodex, sorry\n");
        exit(-1);
    }
    else
    {
        cathelpfile(libdir("lockinfo"), "locked rolodex", 0);
        exit(-1);
    }
}

void rolo_main(int argc, char *argv[])
{
    ALLOCATED_INDEX = 0;

    OPTION_SUMMARY_FLAG = 0;
    OPTION_NOLOCK_FLAG = 0;
    OPTION_OTHERUSER_FLAG = 0;
    NON_OPTION_ARGS = 0;

    int fd, in_use, read_only, rolofd;
    //Bool not_own_rolodex;
    char *user;
    FILE *tempfp;

    clearinit();
    clear_the_screen();

    int c;
    while ((c = getopt(argc, argv, "lsu:")) != -1) {
        switch (c)
        {
        case 'l':
            OPTION_NOLOCK_FLAG = 1;
            break;
        case 's':
            OPTION_SUMMARY_FLAG = 1;
            break;
        case 'u':
            OPTION_OTHERUSER_FLAG = 1;
            OTHER_USERNAME = optarg;
            break;
        case '?':
            if (optopt == 'u') {
                fprintf(stderr, "Illegal syntax using -u option\nusage: %s\n", USAGE);
                roloexit(-1);
            }
            else if (isprint (optopt)) {
                //fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                fprintf(stderr, "illegal option\nusage: %s\n", USAGE);
                roloexit(-1);
            }
            else {
                fprintf(stderr, "illegal option\nusage: %s\n", USAGE);
                roloexit(-1);
            }
            break;
        }
    }

    for (int index = optind; index < argc; index++) {
        NON_OPTION_ARGS++;
        //printf("Non-option argument %s\n", argv[index]);
    }

    /* parse the options and arguments, if any */
    // switch (get_args(argc, argv, T, T))
    // {
    // case ARG_ERROR:
    //     roloexit(-1);
    // case NO_ARGS:
    //     break;
    // case ARGS_PRESENT:
    //     if (ALL_LEGAL != legal_options(LEGAL_OPTIONS))
    //     {
    //         fprintf(stderr, "illegal option\nusage: %s\n", USAGE);
    //         roloexit(-1);
    //     }
    // }

    /* find the directory in which the rolodex file we want to use is */
    if (OPTION_OTHERUSER_FLAG == 1) {
        // not own rolodex
        if (NULL == (user = OTHER_USERNAME)) {
            fprintf(stderr, "Illegal syntax using -u option\nusage: %s\n", USAGE);
            roloexit(-1);
        }
    } else {
        // its our own rolodex
        if (0 == (user = getenv("HOME"))) {
            fprintf(stderr, "Cant find your home directory, no HOME\n");
            roloexit(-1);
        }
    }

    if (OPTION_OTHERUSER_FLAG == 1)
    {
        strcpy(rolodir, home_directory(user));
        if (*rolodir == '\0')
        {
            fprintf(stderr, "No user %s is known to the system\n", user);
            roloexit(-1);
        }
    }
    else {
        if (rolodir != strcpy(rolodir, user)) {
            fprintf(stderr, "strcpy failed.");
            roloexit(-1);
        }
    }

    /* is the rolodex readable? */
    if (0 != access(homedir(ROLODATA), R_OK))
    {

        /* No.  if it exists and we cant read it, that's an error */

        if (0 == access(homedir(ROLODATA), F_OK))
        {
            fprintf(stderr, "Cant access rolodex data file to read\n");
            roloexit(-1);
        }

        /* if it doesn't exist, should we create one? */

        if (OPTION_OTHERUSER_FLAG == 1)
        {
            fprintf(stderr, "No rolodex file belonging to %s found\n", user);
            roloexit(-1);
        }

        /* try to create it */

        if (-1 == (fd = creat(homedir(ROLODATA), 0644)))
        {
            fprintf(stderr, "couldnt create rolodex in your home directory\n");
            roloexit(-1);
        }

        else
        {
            close(fd);
            fprintf(stderr, "Creating empty rolodex...\n");
        }
    }

    /* see if someone else is using it */

    in_use = (0 == access(homedir(ROLOLOCK), F_OK));

    /* are we going to access the rolodex only for reading? */

    if (!(read_only = rolo_only_to_read()))
    {

        /* No.  Make sure no one else has it locked. */

        if (in_use)
        {
            locked_action();
        }

        /* create a lock file.  Catch interrupts so that we can remove */
        /* the lock file if the user decides to abort */

        if (OPTION_OTHERUSER_FLAG != 1)
        {
            if ((fd = open(homedir(ROLOLOCK), O_EXCL | O_CREAT, 00200 | 00400)) < 0)
            {
                fprintf(stderr, "unable to create lock file...\n");
                exit(1);
            }
            rololocked = 1;
            close(fd);
            signal(SIGINT, user_interrupt);
        }

        /* open a temporary file for writing changes to make sure we can */
        /* write into the directory */

        /* when the rolodex is saved, the old rolodex is moved to */
        /* a '~' file, the temporary is made to be the new rolodex, */
        /* and a copy of the new rolodex is made */

        if (NULL == (tempfp = fopen(homedir(ROLOTEMP), "w")))
        {
            fprintf(stderr, "Can't open temporary file to write to\n");
            roloexit(-1);
        }
        fclose(tempfp);
    }

    allocate_memory_chunk(CHUNKSIZE);

    if ((rolofd = open(homedir(ROLODATA), O_RDONLY)) == -1)
    {
        fprintf(stderr, "Can't open rolodex data file to read\n");
        roloexit(-1);
    }

    /* read in the rolodex from disk */
    /* It should never be out of order since it is written to disk ordered */
    /* but just in case... */

    if (!read_only)
        printf("Reading in rolodex from %s\n", homedir(ROLODATA));
    read_rolodex(rolofd);
    close(rolofd);
    if (!read_only)
        printf("%d entries listed\n", rlength(Begin_Rlist));
    if (reorder_file && !read_only)
    {
        fprintf(stderr, "Reordering rolodex...\n");
        rolo_reorder();
        fprintf(stderr, "Saving reordered rolodex to disk...\n");
        save_to_disk();
    }

    /* the following routines live in 'options.c' */

    /* -s option.  Prints a short listing of people and phone numbers to */
    /* standard output */

    if (OPTION_SUMMARY_FLAG == 1)
    {
        print_short();
        exit(0);
    }

    /* rolo <name1> <name2> ... */
    /* print out info about people whose names contain any of the arguments */

    if (NON_OPTION_ARGS > 0)
    {
        print_people(argv, optind);
        exit(0);
    }

    /* regular rolodex program */

    interactive_rolo();
    exit(0);
}
