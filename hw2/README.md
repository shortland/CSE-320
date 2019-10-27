# Homework 2 Debugging and Fixing - CSE 320 - Fall 2019
#### Professor Eugene Stark

### **Due Date: Friday 10/4/2019 @ 11:59pm**

# Introduction

In this assignment you are tasked with updating an old piece of
software, making sure it compiles, and that it works properly
in your VM environment.

Maintaining old code is a chore and an often hated part of software
engineering. It is definitely one of the aspects which are seldom
discussed or thought about by aspiring computer science students.
However, it is prevalent throughout industry and a worthwhile skill to
learn.  Of course, this homework will not give you a remotely
realistic experience in maintaining legacy code or code left behind by
previous engineers but it still provides a small taste of what the
experience may be like.  You are to take on the role of an engineer
whose supervisor has asked you to correct all the errors in the
program, plus add additional functionality.

By completing this homework you should become more familiar
with the C programming language and develop an understanding of:

- How to use tools such as `gdb` and `valgrind` for debugging C code.
- Modifying existing C code.
- C memory management and pointers.

## The Existing Program

Your goal will be to debug and extend an old program called `rolo`,
which was posted to Usenet in March, 1986.  It was written by JP Massar
of Thinking Machines Corporation (who credits Peter Webb for an original
version), and it ran under the 4.2BSD operating system on the
Digital Equipment Corporation VAX computer.  I used this program for awhile
myself starting in about 1989, running under SunOS
(a 4.2BSD derivative) on a Sun3 computer, and running under FreeBSD
(a 4.4BSD derivative) on Intel x86 hardware.
The version I am handing out is very close to the original version,
except that I have made a few changes for this assignment.
First of all, I rearranged the source tree and re-wrote the `Makefile`
to conform to what we are using for the other assignments in this course.
I also introduced a few bugs here and there to make things more interesting
and educational for you :wink:.
Aside from these changes and the introduced bugs, which only involve a few
lines, the code is identical to the original, functioning version.
Although the original program was useful and pretty robust in practical use,
during my analysis of it for this assignment I did find various bugs in it,
which I have left for you to find as well.

The purpose of the `rolo` program is to serve as a computer version of the
"rolodex" card files that were popular in an office setting for maintaining
information about business contacts.

In the default mode of execution (which occurs when you invoke the program
just using `bin/rolo` with no arguments), the program reads in the file
`.rolodex.dat` from the user's home directory, creating an empty file if it
is not present, and then enters a menu-driven, textual interaction that
allows the user to scan, search, and make updates to the data.
A file `.rolodexdata.lock` is used to inform other instances of the `rolo`
program that might potentially be run that the rolodex data file is currently
in use and modifications should not be attempted.  When the program exits,
any changes made to the data are first written to a temporary file, and then some
renaming is performed to rename the original `.rolodex.dat` file to `.rolodex~`
(to serve as a backup file) and to rename the temporary file to
`.rolodex.dat` (to serve as the new version).
Some modes of execution of the program do not have the ability to modify the
data -- in that case no changes are made to any files when the program exits.

The `rolo` program is pretty simple, and you should be able to read the code to
get the understanding you need of how it works in order to be able to complete
the assignment.  Note that you probably don't need to understand every detail.
This is pretty realistic as far as working on legacy software is concerned:
often one would be given code that needs to be updated, ported, or have some
bugs fixed, and this has to be done without (at least initially) having a full
understanding of structure and function of the code.
In this kind of situation, you have to be careful not to make arbitrary changes
to the code that might impact things that you don't fully understand.
Limit your changes to the minimum necessary to achieve the specified objectives.

### Getting Started - Obtain the Base Code

Fetch base code for `hw2` as you did for the previous assignments.
You can find it at this link:
[https://gitlab02.cs.stonybrook.edu/cse320/hw2](https://gitlab02.cs.stonybrook.edu/cse320/hw2).

Once again, to avoid a merge conflict with respect to the file `.gitlab-ci.yml`,
use the following command to merge the commits:

<pre>
  git merge -m "Merging HW2_CODE" HW2_CODE/master --strategy-option=theirs
</pre>

  > :nerd: I hope that by now you would have read some `git` documentation to find
  > out what the `--strategy-option=theirs` does, but in case you didn't :angry:
  > I will say that merging in `git` applies a "strategy" (the default strategy
  > is called "recursive", I believe) and `--strategy-option` allows an option
  > to be passed to the strategy to modify its behavior.  In this case, `theirs`
  > means that whenever a conflict is found, the version of the file from
  > the branch being merged (in this case `HW2_CODE/master`) is to be used in place
  > of the version from the currently checked-out branch.  An alternative to
  > `theirs` is `ours`, which makes the opposite choice.  If you don't specify
  > one of these options, `git` will leave conflict indications in the file itself
  > and it will be necessary for you to edit the file and choose the code you want
  > to use for each of the indicated conflicts.

Here is the structure of the base code:

<pre>
.
├── .gitlab-ci.yml
└── hw2
    ├── doc
    │   ├── README
    │   └── rolo.1
    ├── helplib
    │   ├── addhelp
    │   ├── addinfo
    │   ├── entrymenu
    │   ├── escanhelp
    │   ├── esearchhelp
    │   ├── fieldsearchhelp
    │   ├── lockinfo
    │   ├── mainmenu
    │   ├── moptionhelp
    │   ├── moptionshelp
    │   ├── pickentrymenu
    │   ├── poptionmenu
    │   ├── poptionshelp
    │   ├── updatehelp
    │   └── updatemenu
    ├── include
    │   ├── choices.h
    │   ├── datadef.h
    │   ├── rolodefs.h
    │   ├── rolofiles.h
    │   └── toolsdir
    │       ├── args.h
    │       ├── basics.h
    │       ├── ctools.h
    │       ├── mem.h
    │       ├── menu.h
    │       └── sys5.h
    ├── Makefile
    ├── src
    │   ├── clear.c
    │   ├── io.c
    │   ├── main.c
    │   ├── menuaux.c
    │   ├── operations.c
    │   ├── options.c
    │   ├── rlist.c
    │   ├── rolo.c
    │   ├── search.c
    │   ├── toolsdir
    │   │   ├── args.c
    │   │   ├── ctools.c
    │   │   ├── mem.c
    │   │   └── menu.c
    │   └── update.c
    └── tests
        ├── base_tests.c
        └── rsrc
            ├── dot.rolodex.dat
            ├── invalid_search_test.in
            ├── invalid_search_test.rolo -> dot.rolodex.dat
            ├── search_eof_test.in
            ├── search_eof_test.out
            ├── startup_exit_test.in
            ├── startup_exit_test.out
            ├── unreadable_rolodex_test.in
            ├── valgrind_leak_test.in
            ├── valgrind_leak_test.rolo -> dot.rolodex.dat
            ├── valgrind_uninitialized_test.in
            ├── valgrind_uninitialized_test.rolo -> dot.rolodex.dat
            ├── valgrind_update_test.in
            ├── valgrind_update_test.rolo -> dot.rolodex.dat
            ├── write_error_test.in
            └── write_error_test.rolo -> dot.rolodex.dat
</pre>

The `doc` directory included with the assignment basecode contains the
original documentation files (a `README` and a Unix-style `man` page) that
were distributed with the program.  You can format and read the `man` page
using the following command:

<pre>
nroff -man doc/rolo.1 | less
</pre>

  > :nerd:  Since time immemorial, Unix `man` pages have been written in
  > a typesetting language called `roff` (short for "run-off").
  > The `nroff` program processes `roff` source and produces text formatted
  > for reading in a terminal window.

The `helplib` directory contains files that hold the source text for the
help messages the program will print during execution.  These are not
compiled into the program; rather when the program needs to print a help
message it will open a file in this directory and print its contents to
the terminal.  The `Makefile` has been set so that it passes the absolute
path of this directory as the value of the `ROLOLIB` C preprocessor symbol.
This pathname then gets compiled into the program.  As a result, this
directory must be present and readable when the program is run, otherwise
the help feature will not work properly.

The C source files in the `src` directory are files specific to the `rolo` application.
There is also a subdirectory `src/toolsdir`, which contains C source files for what
appears to be a "C Tools" library that is not specific to a particular application.
The library sources were segregated from the application-specific sources in the original
version and I have left them that way.
Similarly, the header files in the `include` directory are application-specific and
those in the `include/toolsdir` directory are for the library code.
When you look at the application-specific code in `src` you will see that some of it
has been conditionalized on the preprocessor symbol `TMC` having been defined.
It seems that, at Thinking Machines Corporation where the author worked, the C Tools
library had been installed in a system area, so that if the code was compiled with
`TMC` defined, then the compiler would look in a different place for the header files
than if `TMC` was not defined.  The `Makefile` I have provided does not define the `TMC`
symbol.
A couple of files in `src/toolsdir` contain code conditionalized on the preprocessor
symbol `BSD42`.  The `Makefile` I have provided does arrange for that symbol to be
defined and you should leave it that way.
If you look at `src/clear.c` you will see two additional symbols used to conditionalize
the code: `TERMINFO` and `TERMCAP`.  The `Makefile` I have provided does not define
either of these by default.  The purpose of these symbols is to enable the use of
some screen-clearing code from the `curses` library, so that each time a menu is displayed,
the terminal window is cleared first and the menu appears at the top.  We won't be using
this capability, so you should also leave these symbols undefined.

The `tests` directory contains C source code for Criterion tests I have supplied.
The subdirectory `tests/rsrc` contains data files that are used by the tests.

You can modify anything you want in the assignment (except `main.c`), but limit your changes
to the minimum necessary to restore functionality to the program.  Assume that the program
is essentially correct -- it just has a few lingering bugs that need to be fixed.

Before you begin work on this assignment, you should read the rest of this
document.  In addition, we additionally advise you to read the
[Debugging Document](DebuggingRef.md).

# Part 1: Debugging and Fixing

The command line arguments and expected operation of the program are described
by the following "usage" message, which is printed within `rolo_main()` in `rolo.c`:

<pre>
usage: rolo [ person1 person2 ...] [ -l -s -u user ]
</pre>

  > :anguished: What is now the function `rolo_main()` was simply `main()` in the original code.
  > I have changed it so that `main()` now resides in a separate file `main.c` and
  > simply calls `rolo_main()`.  This is to make the structure conform to what is
  > needed in order to be able to use Criterion tests with the program.  **Do not make
  > any modifications to `main.c`.**

The `-l` option causes `rolo` to unlock the rolodex data file as soon as the data
has been read in.
If the `-s` option (which implies the `-l` option) is given, then `rolo` does not
enter an interactive mode of operation, but instead just prints summary information
about each of the entries in the rolodex.
If the `-u` option is given, then it must immediately be followed by a username.
In that case, the program uses the rolodex data file found in that user's home
directory, rather than the one found in the home directory of the user who is
running the program.
If a list of names is given, then `rolo` scans the entries looking for a match
to one of the specified names in the `Name` or `Company` fields.
Instead of the normal user `interaction`, in this case `rolo` presents the matching
entries one at a time, pausing before going on to the next one.

You are to complete the following steps:

1. Clean up the code; fixing any compilation issues, so that it compiles
   without error using the compiler options that have been set for you in
   the `Makefile`.

    > :nerd: When you look at the code, you will likely notice very quickly that
    > (except for `main.c`, which I added) it is written in the "K+R" dialect of C
    > (the ANSI standard didn't exist yet when the program was written).
    > In K+R C, function definitions gave the names of their
    > formal parameters within parentheses after the function name, but the types of the formal
    > parameters were declared between the parameter list and the function body,
    > as in the following example taken from `search.c`:
    >
    > ```c
    >   int select_field_to_search_by (ptr_index,ptr_name) int *ptr_index; char **ptr_name;
    > ```
    >
    > As part of your code clean-up, you should provide ANSI C function prototypes where required.
    > For functions defined in one `.c` file and used in another, their function prototypes should
    > be placed in a `.h` file that is included both in the `.c` file where the function is defined
    > and in the `.c` files where the function is used.  For functions used only in the `.c` file
    > in which they are defined, if a function prototype is required (it will be, if any use
    > of the function occurs before its definition) then the function prototype should be put in
    > that same `.c` file and the function should be declared `static`.
    > It is not necessary to re-write the existing function definitions into ANSI C style; just
    > add the required function prototypes.

    > :nerd: As you clean up the code, you will notice that return types have not been specified
    > for some functions that sometimes return a value and that some functions that have been
    > declared to return a value actually do not.  You should select a prototype for each such
    > function that is appropriate to the way the function was intended to work.  If the function
    > does not actually return any value, use return type `void`.  Keep your eyes open as you
    > make these changes for places where the inconsistencies that existed in the original code
    > might have resulting in hitherto undetected bugs.  Use `git` to keep track of the changes
    > you make and the reasons for them, so that you can later review what you have done and also
    > so that you can revert any changes you made that don't turn out to be a good idea in the end.

    This part of the assignment will likely be a bit tedious.  It will take some time to do it,
    but I think it will be an experience worth having.  As a point of reference, it probably took
    me less than two hours to get the program into a state where it compiled cleanly.  I would
    expect that it will take most students a bit longer than that, but it should not be a days-long
    ordeal.

2. Fix bugs.

    Run the program, exercising the various options, and look for cases in which the program
    crashes or otherwise misbehaves in an obvious way.  We are only interested in obvious
    misbehavior here; don't agonize over program behavior that might just have been the choice
    of the original author.  You should use the provided Criterion tests to help point the
    way here.  Note that these tests will replace the `.rolodex.dat` file in your home directory
    when they run, either with an empty file or else with one of the sample files in the
    `tests/rsrc` directory.

3. Use `valgrind` to identify any memory leaks or other memory access errors.
   Fix any errors you find.

    Run `valgrind` using the following command:

    <pre>
      valgrind --leak-check=full --show-leak-kinds=all [ROLO PROGRAM AND ARGS]
    </pre>

    Note that the bugs that are present will all manifest themselves in some way
    either as program crashes or as memory errors that can be detected by `valgrind`.
    It is not necessary to go hunting for obscure issues with the program output.
    Also, do not make gratuitous changes to the program output, as this will
    interfere with our ability to test your code.

   > :scream:  Note that we are not considering memory that is "still reachable"
   > to be a memory leak.  This corresponds to memory that is in use when
   > the program exits and can still be reached by following pointers from variables
   > in the program.  Although some people consider it to be untidy for a program
   > to exit with "still reachable" memory, it doesn't cause any particular problem.

   > :scream: You are **NOT** allowed to share or post on PIAZZA
   > solutions to the bugs in this program, as this defeats the point of
   > the assignment. You may provide small hints in the right direction,
   > but nothing more.

# Part 2: Adding Features

The base code uses a set of library functions written by the original author
and located in `src/toolsdir/args.c` for processing the command-line arguments.
While the original author clearly put some thought into this argument-processing
library, in the years that have gone by since this program was originally written
there have been more elaborate standardized libraries that have been written
for this purpose.  In particular, the POSIX standard specifies a `getopt()` function,
which you can read about by typing `man 3 getopt`.  A significant advantage to using a
standard library function like `getopt()` for processing command-line arguments,
rather than implementing *ad hoc* code to do it, is that all programs that uses
the standard function will perform argument processing in the same way
rather than having each program implement its own quirks that the user has to
remember.

For this part of the assignment, you are to replace the original argument-processing
code in `rolo` by code that uses the `getopt()` library function.
Once you have done this, your program should not require code from `src/toolsdir/args.c`
(or the header file `include/toolsdir/args.h`) any more.
In your revised program, `rolo_main()` should use `getopt()` to traverse the command-line
arguments.  As it does this, it should set global variables to record which of the
options (`-l`, `-s`, or `-u`) has been seen, and if the `-u` option is seen then a
pointer to the corresponding user name should be saved in a global variable.
Due to the way `getopt()` works, as it traverses the argument list and identifies all
arguments that are options (*i.e.* they start with `-`), the remaining non-option
arguments are permuted to the end, so that once all options have been identified,
what remains are all the non-option arguments, beginning at index `optind` in `argv`.
This behavior will allow you to easily eliminate uses of the original library's
`n_option_args` and `non_option_arg` from the code.
Your revised program should accept the same options and arguments as the original
program, except that the legality constraints on them will conform to the standard
behavior of `getopt()`, rather than the specifications laid out in the comments in
the original `args.c` file.

# Part 3: Testing the Program

For this assignment, you have been provided with a basic set of
Criterion tests to help you debug the program.  We encourage you
to write your own as well as it can help to quickly test inputs to and
outputs from functions in isolation.

In the `tests/base_tests.c` file, there are eight test examples.
You can run these with the following command:

<pre>
    $ bin/rolo_tests -j1
</pre>

To obtain more information about each test run, you can supply the
additional option `--verbose=1`.

  > :nerd: The `-j1` flag specifies that the tests are to be run sequentially, not
  > concurrently, using just one "job".  It is important that you use this flag
  > for this assignment, because the tests will interfere with each other
  > (they all use the same rolodex data file) if they are run concurrently and you
  > will get incorrect results.

The tests have been constructed so that they will point you at most of the
problems with the program.
Each test has one or more assertions to make sure that the code functions
properly.  If there was a problem before an assertion, such as a "segfault",
the test will print the error to the screen and continue to run the
rest of the tests.
Two of the tests use `valgrind` to verify that no memory errors are found.
If errors are found, then you can look at the log file that is left behind by
the test code.
Alternatively, you can better control the information that `valgrind` provides
if you run it manually.

The tests included in the base code are not true "unit tests", because they all
run the program as a black box using `system()`.  For this program, it would also
be possible to test a number of the functions individually using true unit tests.
You are encouraged to try to write some of these so that you learn how to do it
and it is possible that some of the tests we use for grading will be true unit
tests like this.  Note that in the next homework assignment unit tests will likely
be very helpful to you and you will be required to write some of your own.
Criterion documentation for writing your own tests can be found
[here](http://criterion.readthedocs.io/en/master/).

  > :scream: Be sure that you test non-default program options to make sure that
  > the program does not crash when they are used.

# Hand-in Instructions

Ensure that all files you expect to be on your remote
repository are pushed prior to submission.

This homework's tag is: `hw2`

<pre>
$ git submit hw2
</pre>