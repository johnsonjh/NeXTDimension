/* Initialize the PostScript device implementation

Copyright (c) 1983, '84, '85, '86, '87, '88, '89, '90 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

14May90 Jack args to InitMaskCache maximize bmFree->header.length w/o overflow 

*/

#include PACKAGE_SPECS

#include "nulldev.h"
#include "devcommon.h"
#include "genericdev.h"
#include "framedev.h"
#include "framemaskdev.h"

public DevHalftone *defaultHalftone;

public procedure NDDeviceInit()
{
  InitPatternImpl((integer)8000, (integer)40000, (integer)8000,
  (integer)40000);

  IniDevCommon();
  IniGenDevImpl();
  IniFmDevImpl();
  IniMaskDevImpl();
  IniNullDevImpl();
}  /* end of PSDeviceInit */



