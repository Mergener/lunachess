#ifndef LUNA_INPUT_H
#define LUNA_INPUT_H

#include <string>

namespace lunachess::input {

void initializeThread();
void killThread();
bool poll(std::string& out);

}

#endif // LUNA_INPUT_H
