!define APPNAME "ShonaLang"
!define EXEFILE "shonalang.exe"
!define INSTALLDIR "$PROGRAMFILES\ShonaLang"

Outfile "ShonaLangInstaller.exe"
InstallDir ${INSTALLDIR}

Section "Install"
    SetOutPath ${INSTALLDIR}
    File "C:\Users\hp i5\Documents\HCS\Capstone Project\src\Release\tino.exe"

    # Add installation directory to system PATH
    WriteRegExpandStr HKCU "Environment" "PATH" "$INSTDIR;$PATH"

    # Set user environment variable
    WriteRegExpandStr HKCU "Environment" "SHONALANG_PATH" "$INSTDIR"

    # Refresh environment variables
    System::Call 'Kernel32::SendMessageTimeout(i 0xFFFF, i 0x1A, i 0, i 0, i 0, i 5000, *i .r0)'
SectionEnd
