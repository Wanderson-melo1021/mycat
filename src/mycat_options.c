#include <stdio.h>
#include <stdlib.h>

#include "mycat_options.h"

#define MYCAT_OK 0
#define MYCAT_ERR_INVALID_ARG 1

static int mycat_handle_option_char(char opt, MyCatOptions *options) {
    if (opt == 'n') {
        if (!options->number_nonblank_lines) {
            options->number_all_lines = 1;
        }
    } else if (opt == 'b') {
        options->number_nonblank_lines = 1;
        options->number_all_lines = 0;
    } else if (opt == 'E') {
        options->show_ends = 1;
    } else if (opt == 'T') {
        options->show_tabs = 1;
    } else if (opt == 'C') {
        options->cat_prefix_lines = 1;
    } else if (opt == 'c') {
        options->cat_once = 1;
    } else {
        return MYCAT_ERR_INVALID_ARG;
    }

    return MYCAT_OK;
}

void mycat_options_init(MyCatOptions *options) {
    if (!options) {
        return;
    }

    options->number_all_lines = 0;
    options->number_nonblank_lines = 0;
    options->show_ends = 0;
    options->show_tabs = 0;
    options->cat_prefix_lines = 0;
    options->cat_once = 0;
}

int mycat_parse_options(int argc, char *argv[], MyCatOptions *options,
                    int *first_file_index) {
    int i = 1;

    if (!argv || !options || !first_file_index) {
        return MYCAT_ERR_INVALID_ARG;
    }

    mycat_options_init(options);

    while (i < argc) {
        char *arg = argv[i];
        int j = 1;

        if (!arg) {
            break;
        }

        if (arg[0] != '-') {
            break;
        }

        if (arg[1] == '\0') {
            break;
        }

        if (arg[1] == '-' && arg[2] == '\0') {
            i++;
            break;
        }

        while (arg[j] != '\0') {
            if (mycat_handle_option_char(arg[j], options) != MYCAT_OK) {
                fprintf(stderr, "mycat: invalid option -- %c\n", arg[j]);
                return MYCAT_ERR_INVALID_ARG;
            }

            j++;
        }
        
        i++;
    }

    *first_file_index = i;

    return MYCAT_OK;
}

void mycat_print_usage(const char *program_name) {
    char buffer[256];
    int written;

    if (!program_name) {
        return;
    }

    written = snprintf(buffer, sizeof(buffer), 
            "Usage: %s [-n] [-b] [-E] [-T] [-C] [-c] [FILE...]\n"
            "  -n  number all output lines\n"
            "  -b  number nonempty output lines (overrides -n)\n"
            "  -E  display $ at end of each line\n"
            "  -T  display TAB characters as ^I\n"
            "  -C  prefix each output line with a cat emoji\n"
            "  -c  print a cat emoji once before output\n",
            program_name);
    
    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return;
    }

    fputs(buffer, stderr);
}
