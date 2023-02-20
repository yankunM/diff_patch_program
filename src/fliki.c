#include<stdio.h>
#include<stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

static int hunk_getc_helper(HUNK *hp, FILE *in);
static int hunk_next_helper(HUNK *hp, FILE *in);
static int patch_append(HUNK *hp, FILE *in, FILE *out, FILE *diff);
static int patch_delete(HUNK *hp, FILE *in, FILE *out, FILE *diff);
static int patch_change(HUNK *hp, FILE *in, FILE *out, FILE *diff);

static int first_time_running = 1;
static int serial_number = 0;
static int current_char = -1;
static int EOS_returned = 0; //set to on when ERR was returned in hunk_next()
static int ERR_returned = 0; //set to on when ERR was returned in hunk_next()

static int index = 2;
static int line = 0;
static unsigned short int count = 1;

static int line_in = 1; // this keeps track of which line we are at in source file
static int line_out = 1; // this keeps track of which line we are at in output file

static int patch_append(HUNK *hp, FILE *in, FILE *out, FILE *diff){
    int arg1 = hp->old_start;
    int arg2 = hp->old_end;
    int arg3 = hp->new_start;
    int arg4 = hp->new_end;
    if(arg1 != arg2){
        return 1;
    }
    if(arg3 > arg4){
        return 1;
    }
    char c;
    while(line_in <= arg1){
        c = fgetc(in);
        if(c != EOF && global_options != 6 && global_options != 2){
            fputc(c, out);
        }
        if(c == '\n'){
            line_in++;
            line_out++;
        }
    }
    char y = hunk_getc(hp, diff);
    int lines = 1;
    while(!EOS_returned){
        if(y == ERR){
            if(global_options != 6 && global_options != 4){
                hunk_show(hp, stderr);
            }
            return -1;
        }
        if(lines >= arg4-arg3+2){
            if(global_options != 6 && global_options != 4){
                fprintf(stderr, "the number of lines present does not match specified.\n");
                hunk_show(hp, stderr);
            }
            return -1;
        }
        if(y == '\n'){
            lines++;
            line_out++;
        }
        if(c != EOF && global_options != 6 && global_options != 2){
            fputc(y, out);
        }
        y = hunk_getc(hp, diff);
    }
    // printf("Successfully finished append patch.\n");
    return 0;
}

static int patch_change(HUNK *hp, FILE *in, FILE *out, FILE *diff){
    int arg1 = hp->old_start;
    int arg2 = hp->old_end;
    int arg3 = hp->new_start;
    int arg4 = hp->new_end;
    if(arg1 > arg2){
        return 1;
    }
    if(arg3 > arg4){
        return 1;
    }
    char c;
    while(line_in < arg1){
        c = fgetc(in);
        if(c != EOF && global_options != 6 && global_options != 2){
            fputc(c, out);
        }
        if(c == '\n'){
            line_in++;
            line_out++;
        }
    }
    char y = hunk_getc(hp, diff);
    c = fgetc(in);
    int lines = 1;
    while(EOS_returned < 1){
        if(y == ERR){
            if(global_options != 6 && global_options != 4){
                hunk_show(hp, stderr);
            }
            return -1;
        }
        if(lines >= arg2-arg1+2){
            if(global_options != 6 && global_options != 4){
                fprintf(stderr, "the number of lines present does not match specified.\n");
                hunk_show(hp, stderr);
            }
            return -1;
        }
        if(y == '\n'){
            lines++;
            line_in++;
        }
        if(y != c){
            if(global_options != 6 && global_options != 4){
                fprintf(stderr, "the hunk line does not match line in source file.\n");
                hunk_show(hp, stderr);
            }
            return -1;
        }
        y = hunk_getc(hp, diff);
        c = fgetc(in);
    }
    c = ungetc(c, in);
    y = hunk_getc(hp, diff);
    lines = 1;
    while(EOS_returned < 2){
        if(y == ERR){
            if(global_options != 6 && global_options != 4){
                hunk_show(hp, stderr);
            }
            return -1;
        }
        if(lines >= arg4-arg3+2){
            if(global_options != 6 && global_options != 4){
                fprintf(stderr, "the number of lines present does not match specified.\n");
                hunk_show(hp, stderr);
            }
            return -1;
        }
        if(y == '\n'){
            lines++;
            line_out++;
        }
        if(c != EOF && global_options != 6 && global_options != 2){
            fputc(y, out);
        }
        y = hunk_getc(hp, diff);
    }
    // printf("Successfully finished change patch.\n");
    return 0;
}

static int patch_delete(HUNK *hp, FILE *in, FILE *out, FILE *diff){
    int arg1 = hp->old_start;
    int arg2 = hp->old_end;
    int arg3 = hp->new_start;
    int arg4 = hp->new_end;
    // Check validity of headers
    // if old start is not old beginning,
    // then we can't append
    if(arg1 > arg2){
        return 1;
    }
    // if new start is after new end in output,
    // then we cannot append
    if(arg3 > arg4){
        return 1;
    }
    // printf("It reached this point at least.\n");
    // traverse to line 5
    char c;
    while(line_in < arg1){
        c = fgetc(in);
        if(c != EOF && global_options != 6 && global_options != 2){
            fputc(c, out);
        }
        if(c == '\n'){
            line_in++;
            line_out++;
        }
        // this process does not relate to hunks at all
    }
    // check current line out is where we want to append
    // if(line_out-1 != arg3){
    //     printf("argument line in new file is not the line preceding deleted line.\n");
    //     return -1;
    // }
    char y = hunk_getc(hp, diff);
    c = fgetc(in);
    int lines = 1;
    while(!EOS_returned){
        if(y == ERR){
            if(global_options != 6 && global_options != 4){
                hunk_show(hp, stderr);
            }
            return -1;
        }
        // check if line numbers match what header specified
        if(lines >= arg2-arg1+2){
            if(global_options != 6 && global_options != 4){
                fprintf(stderr, "the number of lines present does not match specified.\n");
                hunk_show(hp, stderr);
            }
            return -1;
        }
        if(y == '\n'){
            lines++;
            line_in++;
        }
        // fputc(y, out);
        if(y != c){
            if(global_options != 6 && global_options != 4){
                fprintf(stderr, "the hunk line does not match line in source file.\n");
                hunk_show(hp, stderr);
            }
            return -1;
        }
        y = hunk_getc(hp, diff);
        c = fgetc(in);
    }
    // EOS returned
    // printf("Successfully finished delete patch.\n");
    return 0;
}


int patch(FILE *in, FILE *out, FILE *diff){
    HUNK hunk = {HUNK_NO_TYPE, 0, 0, 0, 0, 0};
    HUNK *hp = &hunk;
    int x;
    x = hunk_next(hp, diff);
    while(x != EOF){
        //checks on x
        if(x == ERR){
            return -1;
        }
        int r;
        if(hp->type == HUNK_APPEND_TYPE){
            r = patch_append(hp, in, out, diff);
            if(r){
                return -1;
            }
        } else if(hp->type == HUNK_DELETE_TYPE){
            r = patch_delete(hp, in, out, diff);
            if(r){
                return -1;
            }
        } else {
            r = patch_change(hp, in, out, diff);
            if(r){
                return -1;
            }
        }
        x = hunk_next(hp, diff);
    }
    // if diff file is empty
    if(diff == NULL){
        return -1;
    }
    // print everything left from in to out
    char c;
    while(c != EOF){
        c=fgetc(in);
        if(c != EOF && global_options != 6 && global_options != 2){
            fputc(c, out);
        }
    }
    return 0;
}

void hunk_show(HUNK *hp, FILE *out) {
    int t = hp->type;
    int arg1 = hp-> old_start;
    int arg2 = hp-> old_end;
    int arg3 = hp-> new_start;
    int arg4 = hp-> new_end;
    if(t == 1){
        //print header
        if(arg1 == arg2){
            if(((arg1+'0') != EOF) && global_options != 6 && global_options != 2){
                fputc(arg1+'0', out);
            }
        } else {
            fprintf(out, "%c,%c", arg1+'0', arg2+'0');
        }
        if(global_options != 6 && global_options != 2){
            fputc('a', out);
        }
        if((arg3 == arg4) && global_options != 6 && global_options != 2){
            fputc(arg3+'0', out);
        } else {
            fprintf(out, "%c,%c", arg3+'0', arg4+'0');
        }
        if(global_options != 6 && global_options != 2){
            fputc('\n', out);
        }
        // parse
        char *parser = hunk_additions_buffer;
        int counter = *(parser);
        parser++;
        counter += *(parser) << 8;
        parser++;
        while(counter > 0){
            fprintf(out, "%c%c", '>', ' ');
            while(counter > 0){
                if((*(parser) != EOF) && global_options != 6 && global_options != 2){
                    fputc(*(parser), out);
                }
                parser++;
                counter--;
            }
            counter = *(parser);
            parser++;
            counter += *(parser) << 8;
            parser++;
            if(parser >= hunk_deletions_buffer + HUNK_MAX){
                break;
            }
        }
        // when counter is zero, check whether parser is at the end
        if(parser >= hunk_additions_buffer + HUNK_MAX){
            fprintf(out, "%c%c%c%c", '.', '.', '.', '\n');
        }
    } else if(t == 2){
        //print header
        if((arg1 == arg2) && global_options != 6 && global_options != 2){
            fputc(arg1+'0', out);
        } else {
            fprintf(out, "%c,%c", arg1+'0', arg2+'0');
        }
        if(global_options != 6 && global_options != 2){
            fputc('d', out);
        }
        if((arg3 == arg4) && global_options != 6 && global_options != 2){
            fputc(arg3+'0', out);
        } else {
            fprintf(out, "%c,%c", arg3+'0', arg4+'0');
        }
        if(global_options != 6 && global_options != 2){
            fputc('\n', out);
        }
        //parsing
        char *parser = hunk_deletions_buffer;
        int counter = *(parser);
        parser++;
        counter += *(parser) << 8;
        parser++;
        while(counter > 0){
            fprintf(out, "%c%c", '<', ' ');
            while(counter > 0){
                if((*(parser) != EOF) && global_options != 6 && global_options != 2){
                    fputc(*(parser), out);
                }
                parser++;
                counter--;
            }
            counter = *(parser);
            parser++;
            counter += *(parser) << 8;
            parser++;
            if(parser >= hunk_deletions_buffer + HUNK_MAX){
                break;
            }
        }
        // when counter is zero, check whether parser is at the end
        if(parser >= hunk_deletions_buffer + HUNK_MAX){
            fprintf(out, "%c%c%c%c", '.', '.', '.', '\n');
        }
    } else if(t == 3){
        //print header
        if(arg1 == arg2){
            fputc(arg1+'0', out);
        } else {
            fprintf(out, "%c,%c", arg1+'0', arg2+'0');
        }
        fputc('c', out);
        if(arg3 == arg4){
            fputc(arg3+'0', out);
        } else {
            fprintf(out, "%c,%c", arg3+'0', arg4+'0');
        }
        fputc('\n', out);
        //parse deletion portion
        char *parser = hunk_deletions_buffer;
        int counter = *(parser);
        parser++;
        counter += *(parser) << 8;
        parser++;
        while(counter > 0){
            fprintf(out, "%c%c", '<', ' ');
            while(counter > 0){
                if(*(parser) != EOF){
                    fputc(*(parser), out);
                }
                parser++;
                counter--;
            }
            counter = *(parser);
            parser++;
            counter += *(parser) << 8;
            parser++;
            if(parser >= hunk_deletions_buffer + HUNK_MAX){
                break;
            }
        }
        if(parser >= hunk_deletions_buffer + HUNK_MAX){
                fprintf(out, "%c%c%c%c", '.', '.', '.', '\n');
        }
        fprintf(out, "%c%c%c%c", '-','-','-','\n');
        // parse
        parser = hunk_additions_buffer;
        counter = *(parser);
        parser++;
        counter += *(parser) << 8;
        parser++;
        while(counter > 0){
            fprintf(out, "%c%c", '>', ' ');
            while(counter > 0){
                if(*(parser) != EOF){
                    fputc(*(parser), out);
                }
                parser++;
                counter--;
            }
            counter = *(parser);
            parser++;
            counter += *(parser) << 8;
            parser++;
            if(parser >= hunk_additions_buffer + HUNK_MAX){
                break;
            }
        }
        // when counter is zero, check whether parser is at the end
        if(parser >= hunk_additions_buffer + HUNK_MAX){
            fprintf(out, "%c%c%c%c", '.', '.', '.', '\n');
        }
    }
}


int hunk_getc(HUNK *hp, FILE *in) {
    // printf("The count is: %d\n", count);
    char c = hunk_getc_helper(hp, in);
    if(c == ERR){
        index = 2;
        line = 0;
        count = 1;
        return c;
    }
    if(c == EOS){
        // traversers reset
        index = 2;
        line = 0;
        count = 1;
        return c;
    }
    if(index + 2 < HUNK_MAX){
        // push into storage
        if(hp->type == HUNK_APPEND_TYPE){
            char *A = hunk_additions_buffer;
            if(c == '\n'){
                *(A + index) = c;
                index++;
                *(A + index) = 0;
                *(A + index + 1) = 0;
                line = index;
                index += 2;
                count = 1;
            } else {
                *(A + index) = c;
                index++;
                count++;
                *(A + line) = count;
                *(A + line + 1) = count >> 8;
                *(A+index) = 0;
                *(A + index + 1) = 0;
            }
        } else if(hp->type == HUNK_DELETE_TYPE){
            char *A = hunk_deletions_buffer;
            if(c == '\n'){
                *(A + index) = c;
                index++;
                *(A + index) = 0;
                *(A + index + 1) = 0;
                line = index;
                index += 2;
                count = 1;
            } else {
                *(A + index) = c;
                index++;
                count++;
                *(A + line) = count;
                *(A + line + 1) = count >> 8;
                *(A+index) = 0;
                *(A + index + 1) = 0;
            }
        } else if(hp->type == HUNK_CHANGE_TYPE){
            // delete
            if(EOS_returned == 0){
                char *A = hunk_deletions_buffer;
                if(c == '\n'){
                    *(A + index) = c;
                    index++;
                    *(A + index) = 0;
                    *(A + index + 1) = 0;
                    line = index;
                    index += 2;
                    count = 1;
                } else {
                    *(A + index) = c;
                    index++;
                    count++;
                    *(A + line) = count;
                    *(A + line + 1) = count >> 8;
                    *(A+index) = 0;
                    *(A + index + 1) = 0;
                }
            } else {
                //insert
                char *A = hunk_additions_buffer;
                if(c == '\n'){
                 *(A + index) = c;
                    index++;
                    *(A + index) = 0;
                    *(A + index + 1) = 0;
                    line = index;
                    index += 2;
                    count = 1;
                } else {
                    *(A + index) = c;
                    index++;
                    count++;
                    *(A + line) = count;
                    *(A + line + 1) = count >> 8;
                    *(A+index) = 0;
                    *(A + index + 1) = 0;
                }
            }
        }
    }
    return c;
}

// unsigned short int count = 0;

// char *A = hunk_additions_buffer;
// char *B = hunk_deletions_buffer;

// char *trA = hunk_additions_buffer;
// char *trB = hunk_deletions_buffer;

// char *begA = hunk_additions_buffer;
// char *begB = hunk_deletions_buffer;


// void store(char object, int size, char *buffer, char *beg, char *tr);

// static char *begA = A;
// static char *begB = B;

// static char *trA = begA + 2;
// static char *trB = begB + 2;

// *(trA) = 0;
// *(trA + 1) = 0;
// *(begA) = count;
// *(begA + 1) = count >> 8;

// *(trB) = 0;
// *(trB + 1) = 0;
// *(begB) = count;
// *(begB + 1) = count >> 8;




// void store(char object, int size, char *buffer, char *beg, char *tr){
//     if(tr < buffer + size - 2){
//         if(object == '\n'){
//             count = 0;
//             *tr = object;
//             beg = tr + 1;
//             tr = beg + 2;
//             *(tr) = 0;
//             *(tr+1) = 0;
//             *(beg) = count;
//             *(beg + 1) = count >> 8;
//         } else {
//             count++;
//             *tr = object;
//             tr++;
//             *(tr) = 0;
//             *(tr+1) = 0;
//             *(beg) = count;
//             *(beg + 1) = count >> 8;
//         }
//     }
// }

static int hunk_next_helper(HUNK *hp, FILE *in){
    int a;
    if(in==NULL){
        // fprintf(stdout, "File path invalid.\n");
        return EOF;
    }
    if((a=fgetc(in)) == EOF){
        // fprintf(stdout, "File end is reached.\n");
        return EOF;
    }
    ungetc(a, in); // resetting header pointer
    HUNK_TYPE typeH = HUNK_NO_TYPE;
    int arg1 = -1;
    int arg2 = -1;
    int arg3 = -1;
    int arg4 = -1;
    int sum = 0;
    a=fgetc(in);
    // int counter = 1;
    if(a < 48 || a > 57){
        if(global_options != 6 && global_options != 4){
            fprintf(stderr, "The first character is not a number.\n");
        }
        return ERR;
    }
    sum = a - '0';
    int comma_b = 0, comma_a = 0;

    // process
    while((a=fgetc(in)) != '\n'){
        if(a == EOF){
            return ERR;
        }
        // counter++;
        // if a is a comma (before)
        if(a == ','){
            comma_b++;
            if(comma_b > 1){
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr,"Too many arguments before type.");
                }
                return ERR;
            }
            // if(sum == 0){
            //     printf("Line number cannot be 0.");
            //     return ERR;
            // }
            arg1 = sum;
            sum = 0;

        // if a is append
        } else if(a == 'a'){
            typeH = HUNK_APPEND_TYPE;
            if(comma_b > 0){
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr,"Too many arguments before append.\n");
                }
                return ERR;
            }
            // no comma before GUARANTEED
            // if(sum == 0){
            //     printf("Line number cannot be 0.");
            //     return ERR;
            // }
            arg1 = sum;
            arg2 = sum;
            sum = 0;
            // copied from change
            while((a = fgetc(in)) != '\n'){
                // counter++;
                if(a == EOF){
                    return ERR;
                }
                if(a == ','){
                    comma_a++;
                    if(comma_a > 1){
                        if(global_options != 6 && global_options != 4){
                            fprintf(stderr, "Too many arguments after append.\n");
                        }
                        return ERR;
                    }
                    // if(sum == 0){
                    //     printf("Line number cannot be 0.");
                    //     return ERR;
                    // }
                    arg3 = sum;
                    sum = 0;
                } else if(a >= '0' && a <= '9'){
                    sum = sum*10;
                    sum += a - '0';
                } else {
                    if(global_options != 6 && global_options != 4){
                        fprintf(stderr, "Not valid argument after append.\n");
                    }
                    return ERR;
                }
            }
            if(sum != 0 && comma_a == 1){
                arg4 = sum;
                sum = 0;
            } else if(sum != 0){
                arg3 = sum;
                arg4 = sum;
                sum = 0;
            } else {
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Need at least one argumnet after append.\n");
                }
                return ERR;
            }
            // a = '\n'
            ungetc(a, in);
        // if a is delete
        } else if(a == 'd'){
            typeH = HUNK_DELETE_TYPE;
            if(sum != 0 && comma_b == 0){
                arg1 = sum;
                arg2 = sum;
            } else if(sum != 0) {
                arg2 = sum;
            } else {
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Line number cannot be 0.\n");
                }
                return ERR;
            }
            sum = 0;

            while((a = fgetc(in)) != '\n'){
                // counter++;
                if(a == EOF){
                    return ERR;
                }
                if(a == ','){
                    if(global_options != 6 && global_options != 4){
                        fprintf(stderr, "Too many arguments after delete.\n");
                    }
                    return ERR;
                } else if(a >= '0' && a <= '9'){
                    sum = sum*10;
                    sum += a - '0';
                } else {
                    if(global_options != 6 && global_options != 4){
                        fprintf(stderr, "Not valid argument after delete.\n");
                    }
                    return ERR;
                }
            }
            // Sum after delete
            arg3 = sum;
            arg4 = sum;
            sum = 0;
            // a = '\n'
            ungetc(a, in);
        } else if(a == 'c'){
            typeH = HUNK_CHANGE_TYPE;
            // sum != 0 GUANRANTEED
            if(sum != 0 && comma_b == 0){
                arg1 = sum;
                arg2 = sum;
            } else if(sum != 0) {
                arg2 = sum;
            } else {
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Line number cannot be 0.\n");
                }
                return ERR;
            }
            sum = 0;
            while((a = fgetc(in)) != '\n'){
                // counter++;
                if(a == EOF){
                    return ERR;
                }
                if(a == ','){
                    comma_a++;
                    if(comma_a > 1){
                        if(global_options != 6 && global_options != 4){
                            fprintf(stderr, "Too many arguments after change.\n");
                        }
                        return ERR;
                    }
                    if(sum == 0){
                        if(global_options != 6 && global_options != 4){
                            fprintf(stderr, "Line number cannot be 0.\n");
                        }
                        return ERR;
                    }
                    arg3 = sum;
                    sum = 0;
                } else if(a >= '0' && a <= '9'){
                    sum = sum*10;
                    sum += a - '0';
                } else {
                    if(global_options != 6 && global_options != 4){
                        fprintf(stderr, "Not valid argument after change.\n");
                    }
                    return ERR;
                }
            }
            if(sum != 0 && comma_a == 1){
                arg4 = sum;
                sum = 0;
            } else if(sum != 0){
                arg3 = sum;
                arg4 = sum;
                sum = 0;
            } else {
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Need at least one argument after change.\n");
                }
                return ERR;
            }
            // a = '\n'
            ungetc(a, in);
        } else if(a >= '0' && a <= '9'){
            sum = sum*10;
            sum += a - '0';
        } else {
            if(global_options != 6 && global_options != 4){
                fprintf(stderr, "Invalid Argument.(s)\n");
            }
            return ERR;
        }
    }
    current_char = '\n';
    serial_number++;
    // printf("HUNK_TYPE: %d\n", typeH);
    // printf("arg1: %d\narg2: %d\narg3: %d\narg4: %d\n", arg1, arg2, arg3, arg4);
    // initialize the hunk structure
    hp->type = typeH;
    hp->serial = serial_number;
    hp->old_start = arg1;
    hp->old_end = arg2;
    hp->new_start = arg3;
    hp->new_end = arg4;
    // printf("The next character we get is: %c\n", a=fgetc(in));
    EOS_returned = 0;
    ERR_returned = 0;
    // line_number++;
    return 0;
}

// Get the header of the next hunk in a diff file.
int hunk_next(HUNK *hp, FILE *in){
    // locate the next/first header
    int a;
    if(in==NULL){
        // fprintf(stdout, "File path invalid.\n");
        return EOF;
    }
    if((a=fgetc(in)) == EOF){
        // fprintf(stdout, "File end is reached.\n");
        return EOF;
    }
    ungetc(a, in); // resetting header pointer
    if(first_time_running){
        first_time_running = 0;
        return hunk_next_helper(hp, in);
    } else {
        while(!EOS_returned){
            hunk_getc(hp, in);
        }
        return hunk_next_helper(hp, in);
    }
}




/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

static int hunk_getc_helper(HUNK *hp, FILE *in){
    if(EOS_returned == 1 && !(hp->type=HUNK_CHANGE_TYPE)){
        if(global_options != 6 && global_options != 4){
            fprintf(stderr, "EOS_was_returned previously. Please call hunk_next() to go to next hunk.\n");
        }
        ERR_returned = 1;
        return ERR;
    }
    if(EOS_returned == 2){
        if(global_options != 6 && global_options != 4){
            fprintf(stderr, "EOS_was_returned previously. Please call hunk_next() to go to next hunk.\n");
        }
        ERR_returned = 1;
        return ERR;
    }
    if(ERR_returned == 1){
        if(global_options != 6 && global_options != 4){
            fprintf(stderr, "ERR_was_returned previously.\n");
        }
        ERR_returned = 1;
        return ERR;
    }
    // Test input files
    int a;
    if(in==NULL){
        if(global_options != 6 && global_options != 4){
            fprintf(stderr, "File path invalid.\n");
        }
        ERR_returned = 1;
        return ERR;
    }
    if(hp==NULL){
        if(global_options != 6 && global_options != 4){
            fprintf(stderr, "Hunk Pointer not-initialized.\n");
        }
        ERR_returned = 1;
        return ERR;
    }
    if(current_char == -1){
        if(global_options != 6 && global_options != 4){
            fprintf(stderr, "Please run hunk_next() first!\n");
        }
        ERR_returned = 1;
        return ERR;
    }
    // APPEND TYPE
    if(hp->type == HUNK_APPEND_TYPE){
        a = fgetc(in);
        if(current_char == '\n'){
            if(a == '>'){
                a = fgetc(in);
                if(a != ' '){
                    if(global_options != 6 && global_options != 4){
                        fprintf(stderr, "Invalid line start.\n");
                    }
                    ERR_returned = 1;
                    return ERR;
                }
                current_char = fgetc(in);
                return current_char;  // SHOULD BE PUSHED INTO ADDITION ARRAY
            } else if(a == EOF){
                // fprintf("End of file reached.\n");
                EOS_returned = 1;
                return EOS;
            } else if(a >= '0' && a <= '9'){
             // printf("End of section reached.\n");
                ungetc(a, in);
                EOS_returned = 1;
                return EOS;
            } else {
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Invalid Hunk file1.\n");
                }
                ERR_returned = 1;
                return ERR;
            }
        } else { // cc != '\n'
            if(a == EOF){
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Unexpected EOF reached.\n");
                }
                ERR_returned = 1;
                return ERR;
            }
            current_char = a;
            return current_char;  // SHOULD BE PUSHED INTO ADDITION ARRAY
        }
    // DELETE TYPE
    } else if(hp->type == HUNK_DELETE_TYPE){
        a = fgetc(in);
        if(current_char == '\n'){
            if(a == '<'){
                a = fgetc(in);
                if(a != ' '){
                    if(global_options != 6 && global_options != 4){
                        fprintf(stderr, "Invalid line start.\n");
                    }
                    ERR_returned = 1;
                    return ERR;
                }
                current_char = fgetc(in);
                return current_char;        // SHOULD BE PUSHED INTO DELETION ARRAY
            } else if(a == EOF){
                // fprintf(stdout, "End of file reached.\n");
                EOS_returned = 1;
                return EOS;
            } else if(a >= '0' && a <= '9'){
                // fprintf(stdout, "End of section reached.\n");
                ungetc(a, in);
                EOS_returned = 1;
                return EOS;
            } else {
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Invalid Hunk file2.\n");
                }
                ERR_returned = 1;
                return ERR;
            }
        } else { // cc != '\n'
            if(a == EOF){
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Unexpected EOF reached.\n");
                }
                ERR_returned = 1;
                return ERR;
            }
            current_char = a;
            return current_char;            // SHOULD BE PUSHED INTO DELETION ARRAY
        }
    } else if(hp->type==HUNK_CHANGE_TYPE){
        // checked
        a = fgetc(in);
        if(current_char == '\n'){
            if(!EOS_returned){
                //deletion section
                if(a == '<'){
                    a = fgetc(in);
                    if(a != ' '){
                        if(global_options != 6 && global_options != 4){
                            fprintf(stderr, "Invalid line start.\n");
                        }
                        ERR_returned = 1;
                        return ERR;
                    }
                    current_char = fgetc(in);
                    return current_char;    // SHOULD BE PUSHED INTO DELETION ARRAY
                } else if(a == EOF){
                    // fprintf(stdout, "End of file reached.\n");
                    ungetc(a, in);
                    EOS_returned = 1;
                    return EOS;
                } else if((a=='-') && ((a=fgetc(in)) == '-') && ((a=fgetc(in)) == '-') && ((a=fgetc(in)) == '\n')){
                    EOS_returned = 1;
                    return EOS;
                } else {
                    if(global_options != 6 && global_options != 4){
                        fprintf(stderr, "Missing addition section.\n");
                    }
                    ERR_returned = 1;
                    return ERR;
                }
            } else {
                //insertion section
                if(a == '>'){
                    a = fgetc(in);
                    if(a != ' '){
                        if(global_options != 6 && global_options != 4){
                            fprintf(stderr, "Invalid line start.\n");
                        }
                      ERR_returned = 1;
                      return ERR;
                    }
                    current_char = fgetc(in);
                    return current_char;        // SHOULD BE PUSHED INTO ADDITION ARRAY
                } else if(a == EOF){
                    // printf("End of file reached.\n");
                    EOS_returned++;
                    return EOS;
                } else if(a >= '0' && a <= '9'){
                    // printf("End of section reached.\n");
                    ungetc(a, in);
                    EOS_returned++;
                    return EOS;
                } else {
                    if(global_options != 6 && global_options != 4){
                        fprintf(stderr, "Invalid Hunk file3.\n");
                    }
                    ERR_returned = 1;
                    return ERR;
                }
            }
        } else { // cc != '\n'
            if(a == EOF){
                if(global_options != 6 && global_options != 4){
                    fprintf(stderr, "Unexpected EOF reached.\n");
                }
                ERR_returned = 1;
                return ERR;
            }
            current_char = a;
            return current_char;
        }
    } else {
        if(global_options != 6 && global_options != 4){
            fprintf(stderr, "Invalid Hunk Type.\n");
        }
        ERR_returned = 1;
        return ERR;
    }
}




