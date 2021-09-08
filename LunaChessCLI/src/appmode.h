#ifndef LUNA_APPMODE_H
#define LUNA_APPMODE_H

#include <vector>

#include "app.h"

namespace lunachess {

class AppMode {
public:
	virtual int run(const std::vector<AppArg>& args) = 0;

	virtual ~AppMode() = default;

protected:
	AppMode() = default;
};

}

#endif // LUNA_APPMODE_H