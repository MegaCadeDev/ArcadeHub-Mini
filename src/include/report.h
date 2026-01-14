#ifndef _REPORT_H_
#define _REPORT_H_

#include "usb.h"
#include "Switch.h"
#include "WiiU.h"

// Switch functions
void set_global_gamepad_report(SwitchIdxOutReport *rpt);
void get_global_gamepad_report(SwitchIdxOutReport *rpt);

// New WiiU functions
void set_global_wiiu_report(WiiUIdxOutReport *src);
void get_global_wiiu_report(WiiUIdxOutReport *dest);

#endif