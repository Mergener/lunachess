#include "../tests.h"

#include "core/staticlist.h"

namespace lunachess::tests {

void testStaticList() {
	StaticList<int, 256> sl;

	for (int i = 0; i < 256; ++i) {
		sl.add(2 * i);
	}

	for (int i = 0; i < 256; ++i) {
		LUNA_ASSERT(sl[i] == i * 2, "Wrong result. Expected " << i * 2 << ", got " << sl[i] << ". (index " << i << ")");
	}

	sl.removeAt(133);
	for (int i = 0; i < 133; ++i) {
		LUNA_ASSERT(sl[i] == i * 2, "Wrong result. Expected " << i * 2 << ", got " << sl[i] << ". (index " << i << ")");
	}
	for (int i = 133; i < 255; ++i) {
		LUNA_ASSERT(sl[i] == ((i + 1) * 2), "Wrong result. Expected " << ((i + 1) * 2) << ", got " << sl[i] << ". (index " << i << ")");
	}

	sl.clear();
	LUNA_ASSERT(sl.count() == 0, "List must be empty.");
}

}