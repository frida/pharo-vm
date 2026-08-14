#pragma once

#include "exportDefinition.h"

#ifndef  DEBUG
# define DEBUG	0
#endif

#define LOG_NONE 		0
#define LOG_ERROR 		1
#define LOG_WARN 		2
#define LOG_INFO 		3
#define LOG_DEBUG		4
#define LOG_TRACE		5

VM_EXPORT void logLevel(int level);
VM_EXPORT int getLogLevel();

VM_EXPORT void logMessage(int level, const char* fileName, const char* functionName, int line, ...);
VM_EXPORT void logAssert(const char* fileName, const char* functionName, int line, char* msg);

VM_EXPORT void registerCurrentThreadToHandleExceptions();
VM_EXPORT void installErrorHandlers();
VM_EXPORT int isLogDebug();

//This variable is set externally by CMAKE
#ifndef SOURCE_PATH_SIZE
# define SOURCE_PATH_SIZE 0
#endif

//FILENAME gives only the filename, as __FILE__ gives all the path
#define __FILENAME__ ((char*)__FILE__ + SOURCE_PATH_SIZE)

#define logTrace(...)	logMessage(LOG_TRACE, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logDebug(...)	logMessage(LOG_DEBUG, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logInfo(...)	logMessage(LOG_INFO, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logWarn(...)	logMessage(LOG_WARN, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logError(...)	logMessage(LOG_ERROR, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define LOG_SIZEOF(expr) logDebug("sizeof("#expr"): %ld", sizeof(expr))

#define logErrorFromErrno(msg) 	logMessageFromErrno(LOG_ERROR, msg, __FILENAME__, __FUNCTION__, __LINE__);
#define logWarnFromErrno(msg) 	logMessageFromErrno(LOG_WARN, msg, __FILENAME__, __FUNCTION__, __LINE__);
#define logDebugFromErrno(msg) 	logMessageFromErrno(LOG_DEBUG, msg, __FILENAME__, __FUNCTION__, __LINE__);

VM_EXPORT void logMessageFromErrno(int level, const char* msg, const char* fileName, const char* functionName, int line);

void error(char* aMessage);


#include <stdio.h>

int vm_printf(const char * format, ... );
void vm_setVMOutputStream(FILE * stream);

// Internal implementations of fprintf and vfprintf, so the different OS can reimplement if needed

VM_EXPORT int fprintf_impl(FILE * stream, const char * format, ... );
VM_EXPORT int vfprintf_impl(FILE * stream, const char * format, va_list arg);

VM_EXPORT void printStatusAfterError();

#ifdef _WIN32

VM_EXPORT char* formatMessageFromErrorCode(int errorCode);
VM_EXPORT void logErrorFromGetLastError(char* msg);

VM_EXPORT char* getErrorLogNameInto(char* nameBuffer, int maxSize);
VM_EXPORT FILE* getErrorLogFile();

VM_EXPORT void openDebugWindow(void* hwnd);
VM_EXPORT void notifyDebugWindow();

#endif
