#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>

#include <list>
#include <set>

#define BUFFER_SIZE 1024

using namespace std;

set <FILE*> fileHandlers;
jmp_buf env;
char modes[12][3] = {"r", "w", "a", "r+", "w+", "a+", "rw", "wr", "wa", "aw", "ar", "ra"};

bool isValidMode(const char* mode) {
    bool flag = false;
    for(int i = 0; i < 12; i++) {
        if(strcmp(modes[i], mode) == 0) {
            flag = true;
            break;
        }
    }
    return flag;
}

// obtain a list of currently open file handles by wrapping:
// fopen def
typedef FILE* (*Func_fopen)(const char*, const char*);
Func_fopen org_fopen = (Func_fopen)dlsym (RTLD_NEXT, "fopen");

// fopen wrapper
FILE* fopen(const char* file, const char* mode) {
    FILE* f = org_fopen(file, mode);
    fileHandlers.insert(f);

    return f;
}

// fdopen def
typedef FILE* (*Func_fdopen)(const char*, const char*);
Func_fdopen org_fdopen = (Func_fdopen)dlsym (RTLD_NEXT, "fdopen");

// fdopen wrapper
FILE* fdopen(const char* file, const char* mode) {
    FILE* f = org_fdopen(file, mode);
    fileHandlers.insert(f);

    return f;
}

// freopen def
typedef FILE* (*Func_freopen)(const char*, const char*, const FILE*);
Func_freopen org_freopen = (Func_freopen)dlsym (RTLD_NEXT, "freopen");

// freopen wrapper
FILE* freopen(const char* file, const char* mode, const FILE* stream) {
    FILE* f = org_freopen(file, mode, stream);
    fileHandlers.insert(f);

    return f;
}

// fclose def
typedef int (*Func_fclose)(const FILE*);
Func_fclose org_fclose = (Func_fclose)dlsym (RTLD_NEXT, "fclose");

// fclose wrapper
int fclose(FILE* f) {
    fileHandlers.erase(f);
    return org_fclose(f);
}

void signalHandler(int signum) {
   longjmp(env, signum);  
}

// check_read_str and check_readwrite_FILE done with if-else reversed

// check for readability of a string (using setjump/longjmp/signal approach)
static bool check_read_str(const char *s) {
    signal(11, signalHandler);
    if(setjmp(env)) {
        signal(11, SIG_DFL);
        return false;
    } else {
        // memory location for string is readable
        while(*s != '\0')
            s++;
        return true;
    }
}

// check read-/writeability of a file handle (using setjump/longjmp/signal approach)
static bool check_readwrite_FILE(FILE *f) {
    signal(11, signalHandler);
    if(setjmp(env)) {
        signal(11, SIG_DFL);
        return false;
    } else {
        // make sure all memory locations are readable and writeable
        char *buffer = (char*)malloc(BUFFER_SIZE);
        int count = 0, readIndex = 0, writeIndex = 0;

        fseek(f, 0, SEEK_END);
        int len = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        while(readIndex != len && writeIndex != len) {
            int read = fread(buffer, 1, BUFFER_SIZE, f);
            fseek(f, 0, readIndex);
            readIndex += read;
            int write = fwrite(buffer, 1, read, f);
            writeIndex += write;
        }

        return true;
    }
}

// WRAP fputs to behave robust
// fputs def
typedef int (*Func_fputs)(const char*, FILE*);
Func_fputs org_fputs = (Func_fputs)dlsym (RTLD_NEXT, "fputs");

// fputs wrapper
int fputs(const char* text, FILE* file) {
    if(file == NULL || check_read_str(text) == false || check_readwrite_FILE(file) == false) {
        return EOF;
    }

    if(fileHandlers.find(file) == fileHandlers.end()) {
        return EOF;
    }
    
    return org_fputs(text, file);
}