#pragma once

#include "exportDefinition.h"

#include <stddef.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "sq.h"
#include "config.h"

#include "sqPlatformSpecific.h"
#include "interpreter.h"

#include "debug.h"
#include "assert.h"

#ifdef _WIN32

#include <windows.h>

#endif

VM_EXPORT const char* getSourceVersion();
VM_EXPORT const char* getVMVersion();

VM_EXPORT void setVMPath(const char* path);

char* getImageName();
void setImageName(const char* name);

int getVMArgumentCount();
char* getVMArgument(int index);

int getImageArgumentCount();
char* getImageArgument(int index);

char** getSystemSearchPaths();
char** getPluginPaths();

void setPharoCommandLineParameters(const char** newVMParams, int newVMParamsCount, const char** newImageParams, int newImageParamsCount);

void initGlobalStructure(void);
void ioInitExternalSemaphores(void);

void ceCheckForInterrupts(void);

sqInt nilObject(void);

VM_EXPORT long long getVMGMTOffset();

VM_EXPORT long aioPoll(long microSeconds);
VM_EXPORT void aioInit(void);

void ioInitTime(void);

VM_EXPORT char* getFullPath(char const *relativePath, char* fullPath, int fullPathSize);
VM_EXPORT void getBasePath(char const *path, char* basePath, int basePathSize);

VM_EXPORT void setProcessArguments(int count, const char** args);
VM_EXPORT void setProcessEnvironmentVector(const char** environment);

// Get information about the process arguments.
// Only available if the process using the VM has given them before.
VM_EXPORT int getProcessArgumentCount();
VM_EXPORT const char** getProcessArgumentVector();
VM_EXPORT const char** getProcessEnvironmentVector();

void * loadModuleHandle(const char *fileName);
sqInt freeModuleHandle(void *module);
void *getModuleSymbol(void *module, const char *symbol);

void *getHandler(sqInt anExternalObject);
void *readAddress(sqInt anExternalAddress);

VM_EXPORT int isVMRunOnWorkerThread();
void setMaxStacksToPrint(sqInt anInteger);

#define allocateMemoryBaseAddress(desiredSize, baseAddress) sqAllocateMemory(desiredSize, desiredSize, baseAddress)
