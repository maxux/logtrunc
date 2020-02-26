#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <unistd.h>

#define SRVMAIN    "/sbin/zinit"
#define MAXSIZE    8 * 1024 * 1024    // 8 MB

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

int watchfile(int inotfd, char *filename) {
    int watchfd;

    debug("[+] trying to watch logfile\n");

    while((watchfd = inotify_add_watch(inotfd, filename, IN_MODIFY)) < 0) {
        warnp(filename);
        usleep(100000);
    }

    return 0;
}

int srvproc(char *action, char *service) {
    int status;
    pid_t pp = fork();

    if(pp < 0) {
        warnp("fork");
        return 1;
    }

    if(pp == 0) {
        debug("[+] dependency: %s %s %s\n", SRVMAIN, action, service);

        char *fargv[] = {SRVMAIN, action, service, 0};
        execv(SRVMAIN, fargv);
        diep("execl");
    }

    debug("[+] waiting for post-process handler to finish\n");
    waitpid(pp, &status, 0);

    return 0;
}

int srvstop(char *service) {
    debug("[+] stopping dependant service: %s\n", service);
    return srvproc("stop", service);
}

int srvstart(char *service) {
    debug("[+] starting dependant services: %s\n", service);
    return srvproc("start", service);
}

int main(int argc, char *argv[]) {
    char *filename = NULL;
    char *service = NULL;
    int inotfd;
    struct inotify_event event;
    struct stat stbuf;
    off_t maxsize = MAXSIZE;

    debug("[+] initializing logtrunc\n");

    if(argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [service]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    filename = argv[1];
    printf("monitoring logfile: %s\n", filename);

    if(argc > 2) {
        service = argv[2];
        debug("[+] service to restart: %s\n", service);
    }

    if((inotfd = inotify_init()) < 0)
        diep("inotify_init");

    // registering file to watch
    watchfile(inotfd, filename);

    debug("[+] monitoring for maxsize: %lu bytes (%.2f KB)\n", maxsize, (maxsize / (1024.0)));

    while(read(inotfd, &event, sizeof(event)) > 0) {
        if(event.mask & IN_IGNORED) {
            debug("[-] watching ignored, file was probably removed\n");
            debug("[-] re-watching file if we can\n");
            watchfile(inotfd, filename);
            continue;
        }

        if(stat(filename, &stbuf) != 0) {
            warnp(filename);
            continue;
        }

        // monitor filesize
        debug("[+] file changed, new size: %lu\n", stbuf.st_size);

        if(stbuf.st_size > maxsize) {
            debug("[+] maximum size reached, truncating\n");

            // stopping dependant service
            if(service)
                srvstop(service);

            // truncate the file
            if(truncate(filename, 0) < 0)
                warnp(filename);

            printf("%s: logfile truncated, restarting dependencies\n", filename);

            // restarting dependant service
            if(service)
                srvstart(service);
        }
    }

    return 0;
}
