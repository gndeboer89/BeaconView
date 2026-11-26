; Inno Setup script for the Windows installer.
; Built in CI: ISCC.exe /DMyAppVersion=0.1.5.1 installer\beaconview.iss
; Expects the deployed app (beaconview.exe, beaconview-songs.exe + Qt DLLs/plugins/qml) in ..\dist

#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif

[Setup]
; Keep this GUID constant: it lets a newer installer upgrade an existing installation.
AppId={{774F34D0-FA23-40D5-AA0D-7E7E8637A5E4}
AppName=BeaconView
AppVersion={#MyAppVersion}
AppVerName=BeaconView {#MyAppVersion}
AppPublisher=BeaconView
DefaultDirName={autopf}\BeaconView
DefaultGroupName=BeaconView
UninstallDisplayIcon={app}\beaconview.exe
SetupIconFile=..\assets\icon\beaconview.ico
OutputDir=..\installer-out
OutputBaseFilename=BeaconView-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Installs for all users when run as administrator, otherwise just for the current user.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
DisableProgramGroupPage=yes
CloseApplications=yes

[Languages]
#if FileExists(CompilerPath + "Languages\Dutch.isl")
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
#endif
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\dist\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\BeaconView"; Filename: "{app}\beaconview.exe"
Name: "{group}\BeaconView Liederen"; Filename: "{app}\beaconview-songs.exe"
Name: "{autodesktop}\BeaconView"; Filename: "{app}\beaconview.exe"; Tasks: desktopicon
Name: "{autodesktop}\BeaconView Liederen"; Filename: "{app}\beaconview-songs.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\beaconview.exe"; Description: "{cm:LaunchProgram,BeaconView}"; Flags: nowait postinstall skipifsilent
