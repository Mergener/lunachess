#ifndef LUNA_APP_H
#define LUNA_APP_H

#include <vector>
#include <string>

namespace lunachess {

struct AppArg {
	std::string argName;
	std::vector<std::string> argParams;
};

}

#endif // LUNA_APP_H