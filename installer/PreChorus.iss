; PreChorus Windows installer (Inno Setup 6). Built by .github/workflows/release.yml.
; iscc /DAppVersion=0.0.1-beta /DStage=<staging dir> /DOutDir=<output dir> installer\PreChorus.iss
#ifndef AppVersion
  #define AppVersion "0.0.1-beta"
#endif
#ifndef Stage
  #define Stage "..\stage"
#endif
#ifndef OutDir
  #define OutDir "..\dist"
#endif

[Setup]
AppId={{6E0B7D3A-4C1F-4B57-9C3E-5D2A8F1B7C21}
AppName=PreChorus
AppVersion={#AppVersion}
AppPublisher=Circuit Drift Labs
AppPublisherURL=https://djshellshoxxx.github.io/circuitdriftlabs/
DefaultDirName={autopf}\Circuit Drift Labs\PreChorus
DefaultGroupName=Circuit Drift Labs
DisableProgramGroupPage=yes
LicenseFile={#Stage}\LICENSE.txt
OutputDir={#OutDir}
OutputBaseFilename=PreChorus-v{#AppVersion}-Windows-Installer
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=PreChorus {#AppVersion}

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: full custom
Name: "clap"; Description: "CLAP plug-in"; Types: full custom
Name: "standalone"; Description: "Standalone application"; Types: full custom

[Files]
Source: "{#Stage}\PreChorus.vst3\*"; DestDir: "{commoncf64}\VST3\PreChorus.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Stage}\PreChorus.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap; Flags: ignoreversion
Source: "{#Stage}\PreChorus.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#Stage}\README.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "{#Stage}\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\PreChorus"; Filename: "{app}\PreChorus.exe"; Components: standalone
Name: "{group}\Uninstall PreChorus"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\PreChorus.exe"; Description: "Launch PreChorus Standalone"; Flags: nowait postinstall skipifsilent; Components: standalone
