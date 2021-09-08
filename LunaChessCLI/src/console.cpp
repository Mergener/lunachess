#include "console.h"

namespace lunachess::console {

static bool s_Raw = false;

bool rawMode() {
	return s_Raw;
}

void rawMode(bool b) {
	s_Raw = b;
}

}