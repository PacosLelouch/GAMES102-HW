// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesCustomVersion.h"

const FGuid FArteriesCustomVersion::GUID(0xa1c045d1, 0xa3214c0e, 0x9978e1fe, 0x67fbe42c);
// Register the custom version with core
FCustomVersionRegistration GRegisterArteriesCustomVersion(FArteriesCustomVersion::GUID, FArteriesCustomVersion::LatestVersion, TEXT("ArteriesVer"));