#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <locale.h>
#include <math.h>

#include <pthread.h>

#include <sys/stat.h>
#include <sys/mman.h>

#include <sys/syscall.h>

#include "dir.h"
#include "tree.h"

typedef struct work_file_args {
    int file_count;
    int ngram_len;
    char **file_list;
} work_file_args;

typedef struct merge_tree_args {
    NGRAM_TREE* t1;
    NGRAM_TREE* t2;
} merge_tree_args;

uint64_t make_logs(NGRAM_TREE*, char*, int);

void *work_files(void *argp){

    int i;
    work_file_args *arg = argp;

    uint64_t count = 0;
    NGRAM_TREE *tree = ngram_tree_init();

    for(i = 0; i < arg->file_count; i ++){
        count += make_logs(tree, arg->file_list[i], arg->ngram_len);
    }

    return tree;
}

void *merge_trees(void *argp){
    merge_tree_args *trees = argp;
    NGRAM_TREE *t1 = trees->t1, *t2 = trees->t2;

    _ngram_merge_trees(t1, t2);
    return NULL;
}


int main(int argc, char *argv[]){
    // int opt, option_index = 0 int n = -1;
    int i, j, n = 4;
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    int max_threads = num_cores;
    int remaining_trees = max_threads;
    // int num_cores = 1;

    /*
    bool warnings = true;
    char *endptr;
    char *input_folder = NULL, *output_folder = NULL;
    */

    /*
    static struct option long_options[] = {
        {"directory", required_argument, 0, 'd'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0},
    };
    */

    pthread_t pth[max_threads];
    work_file_args work[max_threads];
    merge_tree_args merge[max_threads / 2];
    NGRAM_TREE *trees[max_threads];

    DIR_CONTENTS *d;

    setlocale(LC_CTYPE, "de_DE.UTF-8");

    /*
    if(argc == 1){
        fprintf(stderr, "Usage: %s [ -d | --directory ] [ -o | --output ] [ -n ]\n", argv[0]);
        return EXIT_FAILURE;
    }

    while((opt = getopt_long(argc, argv, "d:o:n:y", long_options, &option_index)) != -1){
        switch(opt){
            case 'y':
                warnings = false;
				break;
            case 'd':
                input_folder = optarg;
                break;
            case 'o':
                output_folder = optarg;
                break;
            case 'n':
                n = strtol(optarg, &endptr, 10);
                if(errno == 0 && *endptr){  // dont ask
                    fprintf(stderr, "Invalid argument for -n: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                if(n < 2){
                    fprintf(stderr, "n must be greater than or equal to 2: %i\n", n);
                    return EXIT_FAILURE;
                }
                break;
            case '?':
            default:
                return EXIT_FAILURE;
        }
    }

    if(!input_folder || !output_folder || n < 0){
        fprintf(stderr, "Usage: %s [ -d | --directory ] [ -o | --output ] [ -n ] [ -y ]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if(n > 8 && warnings){
        printf("Calm down rambo! n = %i can take a lot of space. Pass -y to proceed.\n", n);
    }

    if(access(output_folder, R_OK) != 0){
        fprintf(stderr, "Cannot access output folder %s; exiting\n", output_folder);
        return EXIT_FAILURE;
    }
    */

    // char *input_folder = "../../runningkey/gutenberg/pianobaby";
    char *input_folder = "books";
    // char *output_folder = "probs";

    d = dir_read_filenames(input_folder);
    if(!d){
        fprintf(stderr, "shits fucked yo\n");
        return EXIT_FAILURE;
    }

    int f_count = d->file_count;
    for(i = 0; i < max_threads; i++){
        int share = ceil((float)d->file_count / max_threads);
        if(f_count < share){  // last iteration
            share = f_count;
        } else {
            f_count -= share;
        }
        work[i].file_list = malloc(sizeof(char*) * share);
        for(j = 0; j < share; j++){
            work[i].file_list[j] = d->file_list[(i * share) + j];
        }
        work[i].ngram_len = n;
        work[i].file_count = share;
        pthread_create(&pth[i], NULL, work_files, (void*)&work[i]);
    }

    for(int i = 0; i < max_threads; i++){
        pthread_join(pth[i], (void*)&trees[i]);
    }
    dir_free(d);

    while(remaining_trees > 1){
        int half = .5 * remaining_trees;
        for(int i = 0; i < half; i++){
            merge[i].t1 = trees[i];
            merge[i].t2 = trees[i+half];
            pthread_create(&pth[i], NULL, merge_trees, (void*)&merge[i]);
        }

        for(i = 0; i < half; i++){
            pthread_join(pth[i], NULL);
        }
        remaining_trees = half;
    }

    for(i = 0; i < max_threads; i++){
        ngram_tree_free(trees[i]);
        free(work[i].file_list);
    }

    return EXIT_SUCCESS;
}

uint64_t make_logs(NGRAM_TREE *tree, char *filename, int n){
    uint64_t counter = 0, i;

    int *codepoints;
    int *ngram = NULL; 

	struct stat buf;

    int fd = open(filename, O_RDONLY);

	if(!fd){
		return 0;
	}

	stat(filename, &buf);
    // printf("Working on %s\n", filename);

    unsigned char c, following_bytes = 0, chunk;
    size_t size = buf.st_size;

    uint8_t *map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(map == MAP_FAILED){
        printf("map failed lol\n");
        return 0;
    }

    codepoints = malloc(sizeof(int) * size);
    for(i = 0; i < size; i++){
        int collected = 0;
        c = map[i];

        if(!(c & 0x80)){
            continue;
        } else if (!(c & 0x20)){  // we have one byte that follows
            following_bytes = 1;
        } else if (!(c & 0x10)){  // we have two bytes that follow
            following_bytes = 2;
        } else if (!(c & 0x8)){  // we have three bytes that follow
            following_bytes = 3;
        }
        char bits_in_header = (6 - following_bytes);
        char first_bits = ~(0xFF << bits_in_header) & c;
        collected = (first_bits << (following_bytes * 6));
        for(chunk = 1; chunk <= following_bytes; chunk++){
            collected |= (map[i + chunk] & 0x3f) << ((following_bytes - chunk) * 6);
        }
        int lookup = _glyph_lookup(collected);
        if(lookup != -1){
            codepoints[counter] = lookup;
            counter++;
        }
        i += following_bytes;
    }
    close(fd);
    munmap(map, size);
    codepoints = realloc(codepoints, counter * sizeof(int));

    ngram = malloc(sizeof(int) * n);
    for(i = 0; i < counter - n; i++){
        ngram_tree_update(tree, &(codepoints[i]), n);
    }
    free(codepoints);
    free(ngram);
	return counter;
}
