#ifndef MYCAT_OPTIONS_H
#define MYCAT_OPTIONS_H

typedef struct {
    int number_all_lines;
    int number_nonblank_lines;
    int show_ends;
    int show_tabs;
} MyCatOptions;

void mycat_options_init(MyCatOptions *options);

int mycat_parse_options(int argc, char *argv[], 
        MyCatOptions *options, int *first_file_index);

void mycat_print_usage(const char *program_name);

#endif


