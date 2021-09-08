#ifndef LUNA_PERFTMODE_H
#define LUNA_PERFTMODE_H

#include "core/position.h"

#include "appmode.h"

namespace lunachess {

class PerftMode : public AppMode {
public:
	virtual int run(const std::vector<AppArg>& args) override;

private:
	int m_Depth = 4;
	Position m_Position = Position::getInitialPosition();
	bool m_PseudoLegal = false;

	void doArgs(const std::vector<AppArg>& args);
};

}

#endif // LUNA_PERFTMODE_H