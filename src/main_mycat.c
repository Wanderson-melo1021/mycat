#include <stdio.h>
#include <stdlib.h>

#include "mycat.h"
#include "mycat_options.h"

int main(int argc, char *argv[]) {
    MyCatOptions options;
    int first_file_index;
    int status;
    int i;

    status = mycat_parse_options(argc, argv, &options, &first_file_index);
    if (status != MYCAT_OK) {
        mycat_print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (first_file_index >= argc) {
        status = mycat_stream(stdin, "-", &options);
        return (status == MYCAT_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    for (i = first_file_index; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '\0') {
            status = mycat_stream(stdin, "-", &options);
        } else {
            status = mycat_file(argv[i], &options);
        }

        if (status != MYCAT_OK) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
