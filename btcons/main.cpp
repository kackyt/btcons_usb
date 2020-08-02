#include "pch.h"

#include <stdio.h>

extern "C" {
    int piohost_main(int, char*[]);
}

LONG __cdecl
_tmain(
    LONG     Argc,
    LPTSTR * Argv
    )
/*++

Routine description:

    Sample program that communicates with a USB device using WinUSB

--*/
{
    return piohost_main((int)Argc, (char**)Argv);
}
