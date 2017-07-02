typedef struct dir_contents {
    char **file_list;
    int file_count;
} DIR_CONTENTS;


DIR_CONTENTS* dir_read_filenames(char *dirname){
    int slabs = 1, file_count = 0, dir_len = strlen(dirname);
    struct dirent *infile;

    DIR *input = opendir(dirname);
    if(!input){
        return NULL;
    }

    DIR_CONTENTS *d = malloc(sizeof(DIR_CONTENTS));
    d->file_list = malloc(sizeof(char*) * slabs * 100);

    while((infile = readdir(input)) != NULL){
        int filename_len, full_path_len;
        if(strcmp(infile->d_name, ".") == 0 || strcmp(infile->d_name, "..") == 0){
            continue;
        }
        if(slabs * 100 < file_count + 1){
            slabs++;
            d->file_list = realloc(d->file_list, sizeof(char*) * slabs * 100);
        }
        filename_len = strlen(infile->d_name);
        full_path_len = dir_len + 1 + filename_len;  // for '/'
        d->file_list[file_count] = malloc(sizeof(char) * (full_path_len + 1));

        memcpy(d->file_list[file_count], dirname, dir_len + 1);
        strcat(d->file_list[file_count], "/");
        strcat(d->file_list[file_count] + dir_len + 1, infile->d_name);
        file_count++;
    }
    d->file_list = realloc(d->file_list, file_count * sizeof(char*));

    closedir(input);
    d->file_count = file_count;
    return d;
}

void dir_free(DIR_CONTENTS* d){
    int i;
    for(i = 0; i < d->file_count; i++){
        free(d->file_list[i]);
    }
    free(d->file_list);
    free(d);
}
