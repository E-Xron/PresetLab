; Windows installer: puts both VST3s where every DAW looks for them and installs the PresetLab app, in one step.
; Built by the GitHub Actions workflow: ISCC /DSrcDir=<folder with PresetLab.vst3, PresetLab FX.vst3 and PresetLab.exe> /DOutDir=<output>

#define AppName "PresetLab"
#define AppVersion "1.0.0"

[Setup]
AppId={{9A4E2C71-5B3D-4F8A-A1C6-7D0B22000001}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=PresetLab
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
OutputDir={#OutDir}
OutputBaseFilename=PresetLab-Setup
SetupIconFile=PresetLab.ico
UninstallDisplayIcon={app}\PresetLab.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName}

[Types]
Name: "full"; Description: "Instrument, FX plug-in and standalone app"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "PresetLab VST3 instrument (MIDI tracks)"; Types: full custom; Flags: fixed
Name: "fx"; Description: "PresetLab FX VST3 (live guitar, bass or mic on audio tracks)"; Types: full custom
Name: "app"; Description: "PresetLab standalone app"; Types: full

[Files]
Source: "{#SrcDir}\PresetLab.vst3\*"; DestDir: "{commoncf64}\VST3\PresetLab.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SrcDir}\PresetLab FX.vst3\*"; DestDir: "{commoncf64}\VST3\PresetLab FX.vst3"; Components: fx; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SrcDir}\PresetLab.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\PresetLab.exe"; Components: app
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\PresetLab.exe"; Components: app; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: app

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\PresetLab.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\PresetLab FX.vst3"

[Run]
Filename: "{app}\PresetLab.exe"; Description: "Open PresetLab now"; Components: app; Flags: nowait postinstall skipifsilent
