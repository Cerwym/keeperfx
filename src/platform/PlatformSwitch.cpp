#include "pre_inc.h"
#include "platform/PlatformSwitch.h"
#include "post_inc.h"

// TbFileFind opaque stub (Switch does not have file-find yet)
struct TbFileFind {};

const char* PlatformSwitch::GetOSVersion() const    { return "Nintendo Switch"; }
const void* PlatformSwitch::GetImageBase() const    { return nullptr; }
const char* PlatformSwitch::GetWineVersion() const  { return nullptr; }
const char* PlatformSwitch::GetWineHost() const     { return nullptr; }

void PlatformSwitch::InstallExceptionHandler()  {}
void PlatformSwitch::ErrorParachuteInstall()    {}
void PlatformSwitch::ErrorParachuteUpdate()     {}

TbFileFind* PlatformSwitch::FileFindFirst(const char*, TbFileEntry*) { return nullptr; }
int32_t     PlatformSwitch::FileFindNext(TbFileFind*, TbFileEntry*)  { return -1; }
void        PlatformSwitch::FileFindEnd(TbFileFind*)                  {}

void   PlatformSwitch::SetRedbookVolume(SoundVolume) {}
TbBool PlatformSwitch::PlayRedbookTrack(int)         { return false; }
void   PlatformSwitch::PauseRedbookTrack()           {}
void   PlatformSwitch::ResumeRedbookTrack()          {}
void   PlatformSwitch::StopRedbookTrack()            {}

void        PlatformSwitch::SetArgv(int, char**) {}
const char* PlatformSwitch::GetDataPath() const  { return "sdmc:/keeperfx"; }
const char* PlatformSwitch::GetSavePath() const  { return "sdmc:/keeperfx/save"; }
