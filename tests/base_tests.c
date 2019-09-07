#include <stdio.h>
#include <unistd.h>
#include <criterion/criterion.h>
#include <string.h>

#define TEST_REF_DIR "tests/rsrc"
#define TEST_OUTPUT_DIR "tests.out"
#define ROLODEX_LOCK "~/.rolodexdata.lock"
#define ROLODEX_DATA "~/.rolodex.dat"

extern int errors, warnings;

static char program_options[500];
static char user_input_file[500];
static char test_output_subdir[100];
static char test_log_outfile[100];

// IMPORTANT: These tests have to be run sequentially, so it is necessary
// to supply -j1 flag to the test executable.

int setup_test(char *name) {
    char cmd[500];
    sprintf(user_input_file, "%s/%s.in", TEST_REF_DIR, name);
    sprintf(test_log_outfile, "%s/%s.out", TEST_OUTPUT_DIR, name);
    sprintf(test_output_subdir, "%s/%s", TEST_OUTPUT_DIR, name);
    sprintf(cmd, "rm -f %s; rm -fr %s; mkdir -p %s; rm -f %s; rm -f %s; cp -p %s/%s.rolo %s",
	    test_log_outfile, test_output_subdir, test_output_subdir, ROLODEX_LOCK, ROLODEX_DATA,
	    TEST_REF_DIR, name, ROLODEX_DATA);
    fprintf(stderr, "setup(%s)\n", cmd);
    return system(cmd);
}

int run_using_system(char *name, char *pre_cmd, char *valgrind_cmd) {
    char cmd[500];
    setup_test(name);
    sprintf(cmd, "%scat %s | %s bin/rolo %s > %s 2>&1",
	    pre_cmd, user_input_file, valgrind_cmd, program_options, test_log_outfile);
    fprintf(stderr, "run(%s)\n", cmd);
    return system(cmd);
}

void assert_normal_exit(int status) {
    cr_assert_eq(status, 0, "The program did not exit normally (status = 0x%x).\n", status);
}

void assert_error_exit(int status) {
    cr_assert_eq(WEXITSTATUS(status), 0xff,
		 "The program did not exit with status 0xff (status was 0x%x).\n", 
		 status);
}

void assert_outfile_matches(char *name) {
    char cmd[500];
    sprintf(cmd, "diff %s %s/%s.out", test_log_outfile, TEST_REF_DIR, name);
    int err = system(cmd);
    cr_assert_eq(err, 0, "The output was not what was expected (diff exited with status %d).\n", WEXITSTATUS(err));
}

void assert_rolofile_matches(char *name) {
    char cmd[500];
    sprintf(cmd, "diff %s %s/%s.rolo", test_log_outfile, TEST_REF_DIR, name);
    int err = system(cmd);
    cr_assert_eq(err, 0, "The rolodex file was not what was expected (diff exited with status %d).\n", WEXITSTATUS(err));
}

void assert_outdir_matches(char *name) {
    char cmd[500];
    sprintf(cmd, "diff -r %s %s/%s", test_output_subdir, TEST_REF_DIR, name);
    int err = system(cmd);
    cr_assert_eq(err, 0, "The output was not what was expected (diff exited with status %d).\n", WEXITSTATUS(err));
}

void assert_no_valgrind_errors(int status) {
    cr_assert_neq(WEXITSTATUS(status), 37, "Valgrind reported errors -- see %s", test_log_outfile);
}

Test(base_suite, startup_exit_test) {
    char *name = "startup_exit_test";
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_outfile_matches(name);
}

Test(base_suite, write_error_test) {
    char *name = "write_error_test";
    // Limit size of files that can be created to 0 bytes, to force the occurrence
    // of an error while writing the rolodex data file.  In a real situation, this
    // could occur, for example, if the disk was full.
    int err = run_using_system(name, "ulimit -f 0; trap \"\" XFSZ; ", "");
    assert_error_exit(err);
    // NOTE: test log file will be empty due to the ulimit -f 0.
}

Test(base_suite, unreadable_rolodex_test) {
    char *name = "unreadable_rolodex_test";
    // Arrange for the created empty rolodex file to be unreadable.
    int err = run_using_system(name, "umask 700; ", "");
    assert_error_exit(err);
}

Test(base_suite, invalid_search_test) {
    char *name = "invalid_search_test";
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
}

Test(base_suite, search_eof_test) {
    char *name = "search_eof_test";
    int err = run_using_system(name, "", "");
    assert_error_exit(err);
    assert_outfile_matches(name);
}

Test(base_suite, valgrind_uninitialized_test) {
    char *name = "valgrind_uninitialized_test";
    int err = run_using_system(name, "", "valgrind --leak-check=no --undef-value-errors=yes --error-exitcode=37");
    assert_no_valgrind_errors(err);
    assert_normal_exit(err);
}

Test(base_suite, valgrind_leak_test) {
    char *name = "valgrind_leak_test";
    int err = run_using_system(name, "", "valgrind --leak-check=full --error-exitcode=37");
    assert_no_valgrind_errors(err);
    assert_normal_exit(err);
}

Test(base_suite, valgrind_update_test) {
    char *name = "valgrind_update_test";
    int err = run_using_system(name, "", "valgrind --leak-check=full --error-exitcode=37");
    assert_no_valgrind_errors(err);
    assert_normal_exit(err);
}
