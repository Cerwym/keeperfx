#include "pre_inc.h"
#include "platform.h"
#include "bflib_fileio.h"
#include "steam_api.hpp"
#include "bflib_crash.h"
#include "cdrom.h"
#include <string>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include <psp2/kernel/processmgr.h>
#include "post_inc.h"

extern "C" const char * get_os_version() {
    return "PlayStation Vita";
}

extern "C" const void * get_image_base()
{
    return nullptr;
}

extern "C" const char * get_wine_version()
{
    return nullptr; // we're running native
}

extern "C" const char * get_wine_host()
{
    return nullptr; // we're running native
}

extern "C" void install_exception_handler()
{
    // TODO: implement Vita-specific exception handler
}

extern "C" int steam_api_init()
{
    // Steam not supported on PlayStation Vita
    return 0;
}

extern "C" void steam_api_shutdown()
{
    // Steam not supported on PlayStation Vita
}

extern "C" void LbErrorParachuteInstall()
{
    // TODO: implement logging a backtrace on crash
}

extern "C" void LbErrorParachuteUpdate()
{
    // TODO: implement logging a backtrace on crash
}

extern "C" void SetRedbookVolume(SoundVolume)
{
    // CDROM features not applicable on PlayStation Vita
}

extern "C" TbBool PlayRedbookTrack(int)
{
    // CDROM features not applicable on PlayStation Vita
    return false;
}

extern "C" void PauseRedbookTrack()
{
    // CDROM features not applicable on PlayStation Vita
}

extern "C" void ResumeRedbookTrack()
{
    // CDROM features not applicable on PlayStation Vita
}

extern "C" void StopRedbookTrack()
{
    // CDROM features not applicable on PlayStation Vita
}

struct TbFileFind {
	std::string filespec;
	std::string path;
	std::string namebuf;
	DIR * handle = nullptr;
	bool is_pattern = false;
};

bool filespec_is_pattern(const char * filespec) {
	return strchr(filespec, '*') != nullptr;
}

std::string directory_from_filespec(const char * filespec) {
	const auto sep = strrchr(filespec, '/');
	if (sep && sep != filespec) {
		return std::string(filespec, sep - filespec);
	} else {
		return ".";
	}
}

bool find_file(TbFileFind * ff, TbFileEntry * fe) {
	while (true) {
		auto de = readdir(ff->handle);
		if (!de) {
			return false;
		} else if (strcmp(de->d_name, ".") == 0) {
			continue;
		} else if (strcmp(de->d_name, "..") == 0) {
			continue;
		}
		const std::string path = ff->path + "/" + de->d_name;
		ff->namebuf = de->d_name;
		fe->Filename = ff->namebuf.c_str();
		if (ff->is_pattern) {
			if (fnmatch(ff->filespec.c_str(), path.c_str(), FNM_FILE_NAME | FNM_CASEFOLD) != 0) {
				continue;
			}
		}
		struct stat sb;
		if (stat(path.c_str(), &sb) < 0) {
			continue;
		} else if (!S_ISREG(sb.st_mode)) {
			continue;
		}
		return true;
	}
	return false;
}

extern "C" TbFileFind * LbFileFindFirst(const char * filespec, TbFileEntry * fe)
{
	try {
		auto ff = std::make_unique<TbFileFind>();
		ff->is_pattern = filespec_is_pattern(filespec);
		ff->filespec = filespec;
		if (ff->is_pattern) {
			ff->path = directory_from_filespec(filespec);
			ff->handle = opendir(ff->path.c_str());
		} else {
			ff->path = filespec;
			ff->handle = opendir(filespec);
		}
		if (ff->handle) {
			if (find_file(ff.get(), fe)) {
				return ff.release();
			}
		}
	} catch (...) {}
	return nullptr;
}

extern "C" int32_t LbFileFindNext(TbFileFind * ff, TbFileEntry * fe)
{
	try {
		if (find_file(ff, fe)) {
			return 1;
		}
	} catch (...) {}
	return -1;
}

extern "C" void LbFileFindEnd(TbFileFind * ff)
{
	if (ff) {
		closedir(ff->handle);
	}
	delete ff;
}

extern "C" int main(int argc, char *argv[]) {
	return kfxmain(argc, argv);
}
