#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "mycat.h"
#include "mycat_options.h"

static void mycat_report_stdout_error(void) {
    if (errno != 0) {
        fprintf(stderr, "mycat: stdout: %s\n", strerror(errno));
    } else {
        fprintf(stderr, "mycat: stdout: I/O error\n");
    }
}

int main(int argc, char *argv[]) {
    MyCatOptions options;
    int first_file_index;
    int status;
    int exit_code;
    int i;

    status = mycat_parse_options(argc, argv, &options, &first_file_index);
    if (status != MYCAT_OK) {
        mycat_print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (options.cat_once) {
        if (fputs("🐱\n", stdout) == EOF) {
            mycat_report_stdout_error();
            return EXIT_FAILURE;
        }
    }

    if (first_file_index >= argc) {
        status = mycat_stream(stdin, "-", &options);
        return (status == MYCAT_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    exit_code = EXIT_SUCCESS;

    for (i = first_file_index; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '\0') {
            status = mycat_stream(stdin, "-", &options);
        } else {
            status = mycat_file(argv[i], &options);
        }

        if (status != MYCAT_OK) {
            exit_code = EXIT_FAILURE;
        }
    }

    return exit_code;
}
