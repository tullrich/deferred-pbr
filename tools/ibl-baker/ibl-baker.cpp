#include "ibl-baker.h"

FREE_IMAGE_FORMAT getFreeimageFormat(const char *filepath) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileType(filepath, 0);
	if (fif == FIF_UNKNOWN)
	{
		fif = FreeImage_GetFIFFromFilename(filepath);
	}
	if (fif != FIF_UNKNOWN && !FreeImage_FIFSupportsReading(fif))
	{
		return FIF_UNKNOWN;
	}

	return fif;
}
