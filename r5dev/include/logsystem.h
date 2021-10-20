#pragma once

enum class LogType_t : int
{
	SCRIPT_SERVER,
	SCRIPT_CLIENT,
	SCRIPT_UI,
	SCRIPT_WARNING,
	NATIVE
};

struct Log
{
	Log(const std::string Message, const int Ticks, const LogType_t Type)
	{
		this->Message = Message;
		this->Ticks = Ticks;
		this->Type = Type;
	}
	std::string Message = "";
	int Ticks = 1024;
	LogType_t Type = LogType_t::NATIVE;
};

class LogSystem
{
public:
	void AddLog(LogType_t type, std::string text);
	void Update();

private:
	std::array<int, 3> GetLogColorForType(LogType_t type);
	std::vector<Log> m_vLogs;
};

extern LogSystem g_LogSystem;