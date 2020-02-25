#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef RELEASE
    #define debug(...) { printf(__VA_ARGS__); }
#else
    #define debug(...) ((void)0)
#endif

void diep(char *str) {
    perror(str);
    exit(EXIT_FAILURE);
}

void warnp(char *str) {
    perror(str);
}

int postprocess(char *process) {
    pid_t pp = fork();

    if(pp < 0) {
        warnp(process);
        return 1;
    }

    if(pp == 0) {
        debug("[+] executing post process handler\n");

        char *fargv[] = {process, process, 0};
        execv(process, fargv);
        diep("execl");
    }

    int status;
    debug("[+] waiting for post-process handler to finish\n");
    waitpid(pp, &status, 0);

    return 0;
}

int main(int argc, char *argv[]) {
    char *filename = argv[1];
    char *postproc = "/tmp/postproc.sh";
    int inotfd, watchd;
    struct inotify_event event;
    struct stat stbuf;
    off_t maxsize = 5 * 1024; // 1024 * 1024 * 5;

    debug("[+] initializing logtrunc\n");

    if(argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    debug("[+] monitoring logfile: %s\n", filename);

    if((inotfd = inotify_init()) < 0)
        diep("inotify_init");

    if((watchd = inotify_add_watch(inotfd, filename, IN_MODIFY)) < 0)
        diep("inotify_add_watch");

    debug("[+] monitoring for maxsize: %lu\n", maxsize);

    while(read(inotfd, &event, sizeof(event)) > 0) {
        if(stat(filename, &stbuf) != 0) {
            warnp(filename);
            continue;
        }

        // monitor filesize
        debug("[+] file changed, new size: %lu\n", stbuf.st_size);

        if(stbuf.st_size > maxsize) {
            debug("[+] maximum size reached, truncating\n");

            // truncate the file
            if(truncate(filename, 0) < 0)
                warnp(filename);

            // execute post process
            postprocess(postproc);
        }
    }

    return 0;
}
