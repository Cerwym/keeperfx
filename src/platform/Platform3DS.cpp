#include "pre_inc.h"
#include "platform/Platform3DS.h"
#include "post_inc.h"

// TbFileFind opaque stub (3DS does not have file-find yet)
struct TbFileFind {};

const char* Platform3DS::GetOSVersion() const    { return "Nintendo 3DS"; }
const void* Platform3DS::GetImageBase() const    { return nullptr; }
const char* Platform3DS::GetWineVersion() const  { return nullptr; }
const char* Platform3DS::GetWineHost() const     { return nullptr; }

void Platform3DS::InstallExceptionHandler()  {}
void Platform3DS::ErrorParachuteInstall()    {}
void Platform3DS::ErrorParachuteUpdate()     {}

TbFileFind* Platform3DS::FileFindFirst(const char*, TbFileEntry*) { return nullptr; }
int32_t     Platform3DS::FileFindNext(TbFileFind*, TbFileEntry*)  { return -1; }
void        Platform3DS::FileFindEnd(TbFileFind*)                  {}

void   Platform3DS::SetRedbookVolume(SoundVolume) {}
TbBool Platform3DS::PlayRedbookTrack(int)         { return false; }
void   Platform3DS::PauseRedbookTrack()           {}
void   Platform3DS::ResumeRedbookTrack()          {}
void   Platform3DS::StopRedbookTrack()            {}

void        Platform3DS::SetArgv(int, char**) {}
const char* Platform3DS::GetDataPath() const  { return "sdmc:/keeperfx"; }
const char* Platform3DS::GetSavePath() const  { return "sdmc:/keeperfx/save"; }
