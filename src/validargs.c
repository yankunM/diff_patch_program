#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    //Intialize global_options = 0
    global_options = 0;
    // printf("global options updated: %ld\n", global_options);
    if(argc <= 1){return -1;}
    if(*(*(argv + 1)) != '-'){ // No Dash
        if(argc != 2){
            return -1;
        }
        diff_filename = *(argv + 1);
        // printf("File Received: %s\n", diff_filename);
    } else { // Yes Dash
        if(*(*(argv+1)+1) == 'h'){
            global_options = 0 | HELP_OPTION;
            // printf("global options updated: %ld\n", global_options);
            // printf("%s\n", "help option activated");
            return 0;
        } else if(*(*(argv+1)+1) == 'n' || *(*(argv+1)+1) == 'q'){
            if(*(*(argv+argc-1)) == '-'){
                printf("%s\n", "missing file");
                return -1;
            }
            int nn = 0, nq = 0;
            if(*(*(argv+1)+1) == 'n'){
                nn=1;
                // printf("Option -n just appeared. nn = %d\n", nn);
            }else{
                nq=1;
                // printf("Option -q just appeared. nq = %d\n", nq);
            }
            // traverse middle elements
            int i;
            for(i = 2; i < argc - 1; i++){
                if(*(*(argv + i)) == '-'){
                    if(*(*(argv+i)+1) == 'n'){
                        nn = 1;
                        // printf("Option -n just appeared. nn = %d\n", nn);
                    } else if(*(*(argv+i)+1) == 'q'){
                        nq = 1;
                        // printf("Option -q just appeared. nq = %d\n", nq);
                    } else {
                        // printf("Not valid middle terms.\n");
                        return -1;
                    }
                } else {
                    // printf("Not valid middle terms.\n");
                    return -1;
                }
            }
            // Do operations for nn, nq
            if(nn == 1){
                global_options = 0 | NO_PATCH_OPTION;
                // printf("global options updated: %ld\n", global_options);
                if(nq == 1){
                    global_options = global_options | QUIET_OPTION;
                    // printf("global options updated: %ld\n", global_options);
                }
            } else {
                if(nq == 1){
                    global_options = 0 | QUIET_OPTION;
                    // printf("global options updated: %ld\n", global_options);
                }
            }
            // Receive File
            diff_filename = *(argv + argc - 1);
            // printf("File Received: %s\n", diff_filename);
        } else {
            printf("First argument not valid.\n");
            return -1;
        }
    }
    return 0;
}
