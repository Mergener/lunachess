#ifndef LUNA_UCIMODE_H
#define LUNA_UCIMODE_H

#include "appmode.h"

#include <queue>
#include <string>
#include <string_view>
#include <functional>
#include <future>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

#include "lunachess.h"

namespace lunachess {

class UCIMode : public AppMode {
public:
	virtual int run(const std::vector<AppArg>& args) override;

	UCIMode();

private:
	using CmdArgs = std::vector<std::string_view>;
	using CommandFunc = std::function<void(const CmdArgs&)>;

	bool m_PendingIsready = false;
	bool m_Running = false;

	std::queue<std::string> m_InputQueue;
	std::mutex m_InputQueueLock;
	std::vector<std::shared_ptr<std::future<void>>> m_Tasks;
	std::unordered_map<std::string, CommandFunc> m_Commands;

	Position m_Position = Position::getInitialPosition();

	// Search settings:
	int m_Depth = 6;
	int m_WhiteRemainingMs = 9999999;
	int m_BlackRemainingMs = 9999999;
	int m_WhiteIncMs = 0;
	int m_BlackIncMs = 0;
	int m_MovesToGo = 0;


	int uciLoop();
	bool pollInput(std::string& input);
	void startTask(std::function<void()> task);

	// Command handlers:
	void cmdUci(const CmdArgs& args);
	void cmdIsready(const CmdArgs& args);
	void cmdUcinewgame(const CmdArgs& args);
	void cmdPosition(const CmdArgs& args);
	void cmdGo(const CmdArgs& args);
	void cmdQuit(const CmdArgs& args);
};

}

#endif // LUNA_UCIMODE_H
