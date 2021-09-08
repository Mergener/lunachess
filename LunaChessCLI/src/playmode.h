#ifndef LUNA_PLAYMODE_H
#define LUNA_PLAYMODE_H

#include "appmode.h"

#include "core/types.h"
#include "core/position.h"
#include "ai/movepicker.h"

namespace lunachess {

class PlayMode : public AppMode {
public:
	virtual int run(const std::vector<AppArg>& args) override;

	PlayMode() = default;

private:
	ai::MovePicker m_MovePicker;

	Position m_Position = Position::getInitialPosition();
	bool m_Playing = false;

	Side m_BotSide = Side::Black;
	int m_BotDepth = 4;
	bool m_ShowEval = false;
	bool m_ShowTime = false;

	void parseArgs(const std::vector<AppArg>& args);
	void play();
	void humanTurn();
	void computerTurn();
};

}

#endif // LUNA_PLAYMODE_H