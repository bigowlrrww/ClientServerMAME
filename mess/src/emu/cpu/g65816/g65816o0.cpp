#include "emu.h"
#include "debugger.h"
#include "g65816.h"
#include "g65816cm.h"
#define EXECUTION_MODE EXECUTION_MODE_M0X0
#include "g65816op.h"

unsigned int ABSOLUTE_COUNTER=0;
