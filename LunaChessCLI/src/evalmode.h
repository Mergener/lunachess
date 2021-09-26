#ifndef LUNA_EVAL_MODE_H
#define LUNA_EVAL_MODE_H

#include "core/position.h"

#include "appmode.h"

namespace lunachess {

class EvalMode : public AppMode {
public:
	virtual int run(const std::vector<AppArg>& args) override;


private:
    bool m_ShowPv = false;
	int m_Depth = 4;
	Position m_Position = Position::getInitialPosition();

	void doArgs(const std::vector<AppArg>& args);
};

}

#endif // LUNA_EVAL_MODE_H
