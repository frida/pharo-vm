/* MinGW ships winpthreads and MSVC ships nothing, so the few calls the VM
 * makes are answered here on Win32 terms. A thread is its id, which is what
 * sqOSThread is; identity is all the VM compares, and nothing ever joins one.
 */
#ifndef PHAROVM_WIN_PTHREAD_COMPAT_H
#define PHAROVM_WIN_PTHREAD_COMPAT_H

#include <errno.h>
#include <stdlib.h>
#include <windows.h>

/* What a thread gets when nobody asked for more. */
#define PHARO_DEFAULT_STACK_SIZE (1024 * 1024)

typedef DWORD pthread_t;

typedef struct {
    size_t stackSize;
} pthread_attr_t;

typedef struct {
    void *(*start)(void *);
    void *argument;
} PharoThreadStart;

static DWORD WINAPI
pharoThreadMain(LPVOID parameter)
{
    PharoThreadStart start = *(PharoThreadStart *)parameter;

    free(parameter);
    start.start(start.argument);

    return 0;
}

static int
pthread_attr_init(pthread_attr_t *attributes)
{
    attributes->stackSize = 0;
    return 0;
}

static int
pthread_attr_getstacksize(const pthread_attr_t *attributes, size_t *size)
{
    *size = attributes->stackSize != 0
        ? attributes->stackSize
        : PHARO_DEFAULT_STACK_SIZE;
    return 0;
}

static int
pthread_attr_setstacksize(pthread_attr_t *attributes, size_t size)
{
    attributes->stackSize = size;
    return 0;
}

static int
pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
               void *(*start)(void *), void *argument)
{
    PharoThreadStart *carried;
    HANDLE handle;

    carried = malloc(sizeof(*carried));
    carried->start = start;
    carried->argument = argument;

    handle = CreateThread(NULL, attributes != NULL ? attributes->stackSize : 0,
                          pharoThreadMain, carried, 0, thread);
    if (handle == NULL) {
        free(carried);
        return EAGAIN;
    }

    /* Nothing joins these, so the handle goes now and detaching is done. */
    CloseHandle(handle);

    return 0;
}

static int
pthread_detach(pthread_t thread)
{
    return 0;
}

static pthread_t
pthread_self(void)
{
    return GetCurrentThreadId();
}

static int
pthread_equal(pthread_t one, pthread_t other)
{
    return one == other;
}

#endif
