; KeeperFX Installer Script for InnoSetup
; This script creates a professional installer for KeeperFX
; Can be compiled with InnoSetup on Windows or via Wine on Linux

#define MyAppName "KeeperFX"
#define MyAppVersion GetEnv("KEEPERFX_VERSION")
#if MyAppVersion == ""
  #define MyAppVersion "1.3.1.0"
#endif
#define MyAppPublisher "KeeperFX Team"
#define MyAppURL "https://keeperfx.net"
#define MyAppExeName "keeperfx.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
AppId={{8E7A1C35-3F2B-4F4D-8D5E-2B4C8F9A1E6D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=LICENSE
InfoBeforeFile=docs\keeperfx_readme.txt
OutputDir=installer
OutputBaseFilename=keeperfx-{#MyAppVersion}-setup
SetupIconFile=res\icon.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1; Check: not IsAdminInstallMode

[Files]
; Main executables
Source: "pkg\keeperfx.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "pkg\keeperfx_hvlog.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "pkg\keeperfx.map"; DestDir: "{app}"; Flags: ignoreversion
Source: "pkg\keeperfx_hvlog.map"; DestDir: "{app}"; Flags: ignoreversion

; SDL Libraries
Source: "pkg\SDL2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "pkg\SDL2_net.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "pkg\SDL2_mixer.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "pkg\SDL2_image.dll"; DestDir: "{app}"; Flags: ignoreversion

; Configuration
Source: "pkg\keeperfx.cfg"; DestDir: "{app}"; Flags: ignoreversion confirmoverwrite

; Documentation
Source: "pkg\keeperfx_readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion isreadme

; Campaigns
Source: "pkg\campgns\*"; DestDir: "{app}\campgns"; Flags: ignoreversion recursesubdirs createallsubdirs

; Creatures configuration
Source: "pkg\creatrs\*"; DestDir: "{app}\creatrs"; Flags: ignoreversion recursesubdirs createallsubdirs

; Game data
Source: "pkg\fxdata\*"; DestDir: "{app}\fxdata"; Flags: ignoreversion recursesubdirs createallsubdirs

; Levels/Map packs
Source: "pkg\levels\*"; DestDir: "{app}\levels"; Flags: ignoreversion recursesubdirs createallsubdirs

; Language files (if they exist)
Source: "pkg\lang\*"; DestDir: "{app}\lang"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists(ExpandConstant('{src}\pkg\lang'))

; Mods (if they exist)
Source: "pkg\mods\*"; DestDir: "{app}\mods"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists(ExpandConstant('{src}\pkg\mods'))

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{#MyAppName} (Heavy Logging)"; Filename: "{app}\keeperfx_hvlog.exe"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Code]
function DirExists(Path: string): Boolean;
begin
  Result := DirExists(Path);
end;
