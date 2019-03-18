//
// Created by Edward Hyde on 18/03/2019.
//

#include <stdio.h>
#include <magic.h>

int main(int argc, char **argv){
    const char *mime;
    magic_t magic;

    printf("Getting magic from %s\n", argv[1]);

    magic = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic, NULL);
    magic_compile(magic, NULL);
    mime = magic_file(magic, argv[1]);

    printf("%s\n", mime);
    magic_close(magic);

    return 0;
}