#include "cdbase.h"

CDInterface ArchCD = {
CDCORE_ARCH,
"Dummy CD Drive",
DummyCDInit,
DummyCDDeInit,
DummyCDGetStatus,
DummyCDReadTOC,
DummyCDReadSectorFAD
};
