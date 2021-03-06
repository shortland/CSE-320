#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "const.h"
#include "transplant.h"
#include "debug.h"
#include "string_helpers.h"
#include "serialize_helpers.h"
#include "deserialize_helpers.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/*
 * A function that returns printable names for the record types, for use in
 * generating debugging printout.
 */
static char *record_type_name(int i) {
    switch(i) {
    case START_OF_TRANSMISSION:
	return "START_OF_TRANSMISSION";
    case END_OF_TRANSMISSION:
	return "END_OF_TRANSMISSION";
    case START_OF_DIRECTORY:
	return "START_OF_DIRECTORY";
    case END_OF_DIRECTORY:
	return "END_OF_DIRECTORY";
    case DIRECTORY_ENTRY:
	return "DIRECTORY_ENTRY";
    case FILE_DATA:
	return "FILE_DATA";
    default:
	return "UNKNOWN";
    }
}

/*
 * @brief  Initialize path_buf to a specified base path.
 * @details  This function copies its null-terminated argument string into
 * path_buf, including its terminating null byte.
 * The function fails if the argument string, including the terminating
 * null byte, is longer than the size of path_buf.  The path_length variable
 * is set to the length of the string in path_buf, not including the terminating
 * null byte.
 *
 * @param  Pathname to be copied into path_buf.
 * @return 0 on success, -1 in case of error
 */
int path_init(char *name) {
    if (string_length(name) + 1 > sizeof(path_buf)) {
        error("arg string length and null byte (%d) is greater than size of path_buf (%ld)", string_length(name) + 1, sizeof(path_buf));

        return -1;
    }

    copy_string_and_null(name, path_buf);

    path_length = string_length(name);

    debug("path_buf is actually: '%s'; and path_length is: %d", path_buf, path_length);

    return 0;
}

/*
 * @brief  Append an additional component to the end of the pathname in path_buf.
 * @details  This function assumes that path_buf has been initialized to a valid
 * string.  It appends to the existing string the path separator character '/',
 * followed by the string given as argument, including its terminating null byte.
 * The length of the new string, including the terminating null byte, must be
 * no more than the size of path_buf.  The variable path_length is updated to
 * remain consistent with the length of the string in path_buf.
 *
 * @param  The string to be appended to the path in path_buf.  The string must
 * not contain any occurrences of the path separator character '/'.
 * @return 0 in case of success, -1 otherwise.
 */
int path_push(char *name) {
    debug("attempting to path_push with: %s", name);

    if (string_contains_char(name, '/') == 0) {
        debug("string contains needle, error.");

        return -1;
    }

    //debug("cannot exceed size of path_buf %d", string_length(name) + string_length(path_buf) + 1 + 1);
    if (string_length(name) + string_length(path_buf) + 1 + 1 > sizeof(path_buf)) {
        debug("new path_buf would be too large... error");

        return -1;
    }

    // check if path_buf has a lagging / already
    if (position_of_char_from_suffix(path_buf, '/') == string_length(path_buf) - 1) {
        debug("found / at end of path_buf, don't bother appending a new one");

    } else {
        debug("appending a new slash");

        append_string_to_existing(path_buf, "/");
    }

    append_string_to_existing(path_buf, name);
    debug("new path_buf after append is: %s", path_buf);

    // update path_length
    path_length = string_length(path_buf);

    return 0;
}

/*
 * @brief  Remove the last component from the end of the pathname.
 * @details  This function assumes that path_buf contains a non-empty string.
 * It removes the suffix of this string that starts at the last occurrence
 * of the path separator character '/'.  If there is no such occurrence,
 * then the entire string is removed, leaving an empty string in path_buf.
 * The variable path_length is updated to remain consistent with the length
 * of the string in path_buf.  The function fails if path_buf is originally
 * empty, so that there is no path component to be removed.
 *
 * @return 0 in case of success, -1 otherwise.
 */
int path_pop() {
    if (string_length(path_buf) == 0 || path_length == 0) {
        debug("nothing to pop from path, error!");

        return -1;
    }

    // if no '/' in path_buf; set path_buf to nothing
    if (position_of_char_from_suffix(path_buf, '/') == -1) {
        copy_string_and_null("", path_buf);
        path_length = 0;

        debug("path_buf set to nothing: %s", path_buf);

        return 0;
    }

    remove_suffix_at_char(path_buf, '/');
    path_length = string_length(path_buf);

    return 0;
}

/*
 * @brief Deserialize directory contents into an existing directory.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory.  It reads (from the standard input) a sequence of DIRECTORY_ENTRY
 * records bracketed by a START_OF_DIRECTORY and END_OF_DIRECTORY record at the
 * same depth and it recreates the entries, leaving the deserialized files and
 * directories within the directory named by path_buf.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * each of the records processed.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including depth fields in the records read that do not match the
 * expected value, the records to be processed to not being with START_OF_DIRECTORY
 * or end with END_OF_DIRECTORY, or an I/O error occurs either while reading
 * the records from the standard input or in creating deserialized files and
 * directories.
 */
int deserialize_directory(int depth) {
    int recordType;
    // int isDirOpen = depth - 1;

    while ((recordType = validate_record_return_type(depth)) != -1) {
        debug("START OF WHILE LOOP; PATH %s; LENGTH: %d", path_buf, path_length);

        if (recordType == 2) {
            if (read_directory_start() == -1) {
                error("unable to read directory start at depth %d", depth);

                return -1;
            }

            debug("successfully read a start of directory at depth %d", depth);

            // isDirOpen++;

            continue;
        }

        if (recordType == 4) {
            debug("successfully read a directory entry");

            Metadata m;

            if ((m = read_dir_entry_data()).error == -1) {
                error("unable to read record metadata");

                return -1;
            }

            if (read_stdin_into_name(name_buf, 0, m.size) == -1) {
                error("unable to read name from stdin");

                return -1;
            }

            if (path_push(name_buf) == -1) {
                error("unable to push name_buf into path_buf");

                return -1;
            } else {
                debug("path_buf is now %s", path_buf);
            }

            path_length = string_length(path_buf);
            debug("the path_buf is now: %s", path_buf);

            if (S_ISDIR(m.permissions)) {
                DIR* dir = opendir(path_buf);

                if (dir) {
                    debug("directory exists; check if clobber is present");

                    if (global_options & (1 << 3)) {
                        debug("-c, clobber was set, and dir already exists...\
                            shouldnt overwrite dirs, so continue...");

                        return deserialize_directory(depth + 1);
                    } else {
                        error("directory exists & wasn't given -c");

                        return -1;
                    }

                    if (closedir(dir) == -1) {
                        error("unable to close dir");

                        return -1;
                    }
                } else if (ENOENT == errno) {
                    debug("directory not found, attempting to create it.");

                    if (create_dir(path_buf) == -1) {
                        error("unable to create directory in path_buf (overwrite not enabled, but shouldn't exist)");

                        return -1;
                    }

                    if (chmod(path_buf, m.permissions & 0777) == -1) {
                        error("unable to set permissions for the specfied directory");

                        return -1;
                    }

                    return deserialize_directory(depth + 1);
                } else {
                    error("opendir failed for unknown reason");

                    return -1;
                }
            } else if (S_ISREG(m.permissions)) {
                debug("its a file!");

                if (deserialize_file(depth) == -1) {
                    error("unable to deserialize file");

                    return -1;
                }

                if (chmod(path_buf, m.permissions & 0777) == -1) {
                    error("unable to change permissions for the specified file");

                    return -1;
                }

                if (path_pop() == -1) {
                    error("unable to pop from path");

                    return -1;
                }
            } else {
                error("unknown type from permissions %d, remaining size: %lu", m.permissions, m.size);

                return -1;
            }
        } else if (recordType == 3) {
            // if (isDirOpen < 0) {
            //     error("there is no START_OF_DIRECTORY record to match this END_OF_DIRECTORY record");

            //     return -1;
            // }

            if (read_directory_end() == -1) {
                error("unable to read directory end at depth %d", depth);

                return -1;
            }

            if (path_pop() == -1) {
                error("unable to pop from path");

                return -1;
            }

            if (depth == 1) {
                debug("read the final END_OF_DIRECTORY record");

                return 0;
            }

            // isDirOpen--;

            return deserialize_directory(depth - 1);
        } else {
            error("record type not expected %d", recordType);

            return -1;
        }
    }

    return -1;
}

/*
 * @brief Deserialize the contents of a single file.
 * @details  This function assumes that path_buf contains the name of a file
 * to be deserialized.  The file must not already exist, unless the ``clobber''
 * bit is set in the global_options variable.  It reads (from the standard input)
 * a single FILE_DATA record containing the file content and it recreates the file
 * from the content.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * the FILE_DATA record.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including a depth field in the FILE_DATA record that does not match
 * the expected value, the record read is not a FILE_DATA record, the file to
 * be created already exists, or an I/O error occurs either while reading
 * the FILE_DATA record from the standard input or while re-creating the
 * deserialized file.
 */
int deserialize_file(int depth) {
    if (access(path_buf, F_OK) != -1) {
        debug("file already exists, check if clobber is present.");

        if (global_options & (1 << 3)) {
            debug("clobber is set, overwrite file that exists");

            if (read_file_data_make_file(depth, path_buf) == -1) {
                error("unable to read file data and recreate the file");

                return -1;
            }
        } else {
            error("file exists, and clobber is not present");

            return -1;
        }
    } else {
        debug("file at %s not found. should attempt to create it.", path_buf);

        if (read_file_data_make_file(depth, path_buf) == -1) {
            error("unable to read file data and recreate the file");

            return -1;
        }
    }

    return 0;
}

/*
 * @brief  Serialize the contents of a directory as a sequence of records written
 * to the standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory to be serialized.  It serializes the contents of that directory as a
 * sequence of records that begins with a START_OF_DIRECTORY record, ends with an
 * END_OF_DIRECTORY record, and with the intervening records all of type DIRECTORY_ENTRY.
 *
 * @param depth  The value of the depth field that is expected to occur in the
 * START_OF_DIRECTORY, DIRECTORY_ENTRY, and END_OF_DIRECTORY records processed.
 * Note that this depth pertains only to the "top-level" records in the sequence:
 * DIRECTORY_ENTRY records may be recursively followed by similar sequence of
 * records describing sub-directories at a greater depth.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open files, failure to traverse directories, and I/O errors
 * that occur while reading file content and writing to standard output.
 */
int serialize_directory(int depth) {
    if (write_record_dir_start(depth) == -1) {
        debug("write record dir start error!");

        return -1;
    }

    struct dirent *de;
    DIR *dir = opendir(path_buf);

    if (dir == NULL) {
        error("error dir was null!");

        return -1;
    }

    /**
     * For holding metadata/chmod
     */
    struct stat stat_buf;

    while ((de = readdir(dir)) != NULL) {
        if ((string_equals(de->d_name, ".") == 0) || (string_equals(de->d_name, "..") == 0)) {
            debug("skipping . or ..");

            continue;
        }

        // push the read file/dir into path_buf
        if (path_push(de->d_name) == -1) {
            error("unable to append name to path");

            return -1;
        }

        // get metadata about current file in path_buf
        // change path_buf every file we recurse through
        if (stat(path_buf, &stat_buf) == -1) {
            error("unable to get metadata about current file in path_buf");

            return -1;
        }

        debug("THE TYPE IS: %d", stat_buf.st_mode);

        if (S_ISDIR(stat_buf.st_mode)) {
            if (write_record_dir_entry(stat_buf.st_mode, stat_buf.st_size, depth, de->d_name) == -1) {
                error("unable to write record dir entry");

                return -1;
            }

            if (serialize_directory(depth + 1) == -1) {
                error("unable to recursively execute serialize_directory()");

                return -1;
            }
        } else if (S_ISREG(stat_buf.st_mode)) {
            if (write_record_dir_entry(stat_buf.st_mode, stat_buf.st_size, depth, de->d_name) == -1) {
                error("unable to write dir entry");

                return -1;
            }

            if (serialize_file(depth, stat_buf.st_size) == -1) {
                error("unable to serialize file");

                return -1;
            }
        } else {
            error("unknown file type");

            return -1;
        }

        if (path_pop() == -1) {
            error("unable to pop from path");

            return -1;
        }
    }

    if (closedir(dir) == -1) {
        error("unable to close dir");

        return -1;
    }

    if (write_record_dir_end(depth) == -1) {
        error("write record dir end error");

        return -1;
    }

    return 0;
}

/*
 * @brief  Serialize the contents of a file as a single record written to the
 * standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * file to be serialized.  It serializes the contents of that file as a single
 * FILE_DATA record emitted to the standard output.
 *
 * @param depth  The value to be used in the depth field of the FILE_DATA record.
 * @param size  The number of bytes of data in the file to be serialized.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open the file, too many or not enough data bytes read
 * from the file, and I/O errors reading the file data or writing to standard output.
 */
int serialize_file(int depth, off_t size) {
    if (size == -1) {
        error("write size is negative");

        return -1;
    }

    if (write_record_file_data(depth, path_buf, size) == size) {
        debug("sucessfully wrote FILE_DATA record");

        return 0;
    }

    error("serialize FILE_DATA record failed");

    return -1;
}

/**
 * @brief Serializes a tree of files and directories, writes
 * serialized data to standard output.
 * @details This function assumes path_buf has been initialized with the pathname
 * of a directory whose contents are to be serialized.  It traverses the tree of
 * files and directories contained in this directory (not including the directory
 * itself) and it emits on the standard output a sequence of bytes from which the
 * tree can be reconstructed.  Options that modify the behavior are obtained from
 * the global_options variable.
 *
 * @return 0 if serialization completes without error, -1 if an error occurs.
 */
int serialize() {
    debug("entered serialize function");

    if (write_record_start() == -1) {
        error("unable to write record start");

        return -1;
    }

    if (serialize_directory(1) == -1) {
        error("serialize dir error!");

        return -1;
    }

    if (write_record_end() == -1) {
        error("unable to write record end");

        return -1;
    }

    return 0;
}

/**
 * @brief Reads serialized data from the standard input and reconstructs from it
 * a tree of files and directories.
 * @details  This function assumes path_buf has been initialized with the pathname
 * of a directory into which a tree of files and directories is to be placed.
 * If the directory does not already exist, it is created.  The function then reads
 * from from the standard input a sequence of bytes that represent a serialized tree
 * of files and directories in the format written by serialize() and it reconstructs
 * the tree within the specified directory.  Options that modify the behavior are
 * obtained from the global_options variable.
 *
 * @return 0 if deserialization completes without error, -1 if an error occurs.
 */
int deserialize() {
    debug("entered deserialize function");

    DIR* dir = opendir(path_buf);

    if (dir) {
        debug("specified directory in path_buf already exists.");

        closedir(dir);
    } else if (ENOENT == errno) {
        debug("specified path_buf directory doesn't exist, attempting to create it.");

        if (mkdir(path_buf, 0700) == -1) {
            error("unable to create directory specified by path_buf");

            return -1;
        }
    } else {
        error("unable to open directory for unknown reason.");

        return -1;
    }

    if (read_record_start() == -1) {
        error("error reading record start");

        return -1;
    }

    if (deserialize_directory(1) == -1) {
        error("unable to deserialize directory");

        return -1;
    }

    if (read_record_end() == -1) {
        error("error reading record end");

        return -1;
    }

    return 0;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv) {
    debug("this is argc %d", argc);

    global_options = 0x0;

    if (argc == 1) {
        error("args are required!");

        return -1;
    }

    for (int i = 0; i < argc; ++i) {
        if (*(*(argv + i)) == 45) {
            // -h
            if (*((*(argv + i)) + 1) == 104) {
                if (*((*(argv + i)) + 2) == 0) {
                    // -h must be the first argument after program name
                    if (i == 1) {
                        debug("-h flag found!");

                        global_options |= 1 << 0;

                        return 0;
                    } else {
                        return -1;
                    }
                }
            }

            // -s
            if (*((*(argv + i)) + 1) == 115) {
                if (*((*(argv + i)) + 2) == 0) {
                    debug("-s flag found!");

                    global_options |= 1 << 1;
                }
            }

            // -d
            if (*((*(argv + i)) + 1) == 100) {
                if (*((*(argv + i)) + 2) == 0) {
                    debug("-d flag found!");

                    global_options |= 1 << 2;
                }
            }

            // -c but only after -hsd, technically only valid after -d
            if (*((*(argv + i)) + 1) == 99) {
                if (*((*(argv + i)) + 2) == 0) {
                    debug("-c flag found!");

                    if (global_options & (1 << 2)) {
                        debug("-d was previously found, so -c can be set.");

                        global_options |= 1 << 3;
                    } else {
                        error("-d wasn't previously found, so -c cannot be set.");

                        return -1;
                    }
                }
            }

            // -p but only after -hsd and with a parameter following it, can apply for -s or -d
            if (*((*(argv + i)) + 1) == 112) {
                if (*((*(argv + i)) + 2) == 0) {

                    debug("-p flag found!");

                    if (global_options & (1 << 1) || global_options & (1 << 2)) {

                        debug("-d|-s was previously found, so -p can be set.");

                        if (i + 1 < argc) {
                            // since -c can appear after -p, make sure that the next direct paramter isn't exactly '-c'
                            if (string_length(*(argv + i + 1)) == 2) {
                                if (*((*(argv + i + 1)) + 0) == 45) {
                                    if (*((*(argv + i + 1)) + 1) == 99) {
                                        error("next parameter is -c, not a real path!!!");

                                        return -1;
                                    }
                                }
                            }

                            // set the name_buf buffer to the contents of the parameter after -p
                            int j;
                            for (j = 0; j < string_length(*(argv + i + 1)); ++j) {
                                *(name_buf + j) = *((*(argv + i + 1)) + j);
                            }

                            // set last to null terminator just in-case...
                            *(name_buf + j) = '\0';

                            debug("contents of name_buf set to: %s", name_buf);
                        } else {
                            error("-p doesn't have anything after it");

                            return -1;
                        }
                    } else {
                        error("-d|-s wasn't previously found, so -p cannot be set.");

                        return -1;
                    }
                }
            }

            debug("this is the char-code after '-': %d", *((*(argv + i) + 1)) );
        }

        debug("this is the contents of the argument %s", *(argv + i));
    }

    if ((global_options & (1 << 1)) || (global_options & (1 << 2))) {
        debug("we got either -s or -d! (good so far!)");

        if ((global_options & (1 << 1)) && (global_options & (1 << 2))) {
            error("both -s and -d were set! (bad!)");

            return -1;
        } else {
            debug("only 1 set, good!");

            return 0;
        }
    } else {
        error("neither -s nor -d was specified");

        return -1;
    }

    return 0;
}
