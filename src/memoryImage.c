#include "pharovm/pharo.h"
#include "pharovm/imageAccess.h"
#include "pharovm/memoryImage.h"

#include <string.h>

static sqInt memoryImageClose(sqImageFile f);
static sqImageFile memoryImageOpen(const char *fileName, char *mode);
static long int memoryImagePosition(sqImageFile f);
static size_t memoryImageRead(void *ptr, size_t sz, size_t count, sqImageFile f);
static int memoryImageSeek(sqImageFile f, long int pos);
static int memoryImageSeekEnd(sqImageFile f, long int pos);
static size_t memoryImageWrite(void *ptr, size_t sz, size_t count, sqImageFile f);
static int memoryImageExists(const char *aPath);
static void memoryImageReportProgress(size_t totalSize, size_t currentSize);
static int memoryImageIsDirectory(const char *aPath);

static const char *imageData = NULL;
static size_t imageSize = 0;
static size_t imagePosition = 0;

static FileAccessHandler memoryFileAccessHandler = {
	memoryImageClose,
	memoryImageOpen,
	memoryImagePosition,
	memoryImageRead,
	memoryImageSeek,
	memoryImageSeekEnd,
	memoryImageWrite,
	memoryImageExists,
	memoryImageReportProgress,
	memoryImageIsDirectory
};

EXPORT(void)
useImageInMemory(const void *data, size_t size)
{
	imageData = data;
	imageSize = size;
	imagePosition = 0;

	setFileAccessHandler(&memoryFileAccessHandler);
}

static sqInt
memoryImageClose(sqImageFile f)
{
	return 0;
}

static sqImageFile
memoryImageOpen(const char *fileName, char *mode)
{
	imagePosition = 0;

	return (sqImageFile) imageData;
}

static long int
memoryImagePosition(sqImageFile f)
{
	return imagePosition;
}

static size_t
memoryImageRead(void *ptr, size_t sz, size_t count, sqImageFile f)
{
	size_t available = (imageSize - imagePosition) / sz;
	size_t read = count < available ? count : available;

	memcpy(ptr, imageData + imagePosition, read * sz);
	imagePosition += read * sz;

	sqImageReportProgress(imageSize, imagePosition);

	return read;
}

static int
memoryImageSeek(sqImageFile f, long int pos)
{
	imagePosition = pos;

	return 0;
}

static int
memoryImageSeekEnd(sqImageFile f, long int pos)
{
	imagePosition = imageSize + pos;

	return 0;
}

static size_t
memoryImageWrite(void *ptr, size_t sz, size_t count, sqImageFile f)
{
	return 0;
}

static int
memoryImageExists(const char *aPath)
{
	return 1;
}

static void
memoryImageReportProgress(size_t totalSize, size_t currentSize)
{
}

static int
memoryImageIsDirectory(const char *aPath)
{
	return 0;
}
