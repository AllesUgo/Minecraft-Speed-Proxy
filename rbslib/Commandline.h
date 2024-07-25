#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <list>
#include <functional>
#include <tuple>


namespace RbsLib::Command
{
	class CommandLine
	{
	private:
		std::vector<std::string> args;
	public:
		bool Parse(const std::string& command);
		CommandLine() = default;
		CommandLine(int argc, const char** argv);
		bool IsFlagExist(const std::string& flag);
		std::string GetNextToken(const std::string& flag) const;
		std::string operator[](int index) const;
		std::string operator[](const std::string& flag) const;
		int GetSize() const;
		void Clear();
		void Push(const std::string& arg);
	};

	class CommandExecuter
	{
	public:
		typedef std::map<std::string, std::list<std::string>> Args;
		class CommandNode
		{
		private:
			std::map<std::string, std::tuple<int, CommandNode*, std::string>> children;
			std::function<void(const std::map<std::string, std::list<std::string>>&)> func = nullptr;
			std::list<CommandNode*>* nodes;
			bool enable_auto_help=false;
			const std::function<void(const std::string& str)>* output_callback;
		public:
			auto operator[](const std::string& str)->CommandNode&;
			void operator()(const std::map<std::string, std::list<std::string>>& args);
			void SetFunction(const std::function<void(const std::map<std::string, std::list<std::string>>&)>& func);
			auto CreateSubOption(const std::string& option, int have_extra_arg, const std::string description="", bool is_enable_auto_help =false, const std::function<void(const std::map<std::string, std::list<std::string>>&)>& func=nullptr) -> CommandNode&;
			auto CreateSelfReferenceOption(const std::string& option,int extra_args_num, const std::string& description) -> CommandNode&;
			CommandNode(std::list<CommandNode*>* nodes,const std::function<void(const std::string& str)>*callback);
			int OptionNum(const std::string& option);
			bool IsFunctionExist() const noexcept;
		};
		auto operator[](const std::string& str)->CommandNode&;
		CommandExecuter();
		auto CreateSubOption(const std::string& option, int have_extra_arg, const std::string description = "", bool is_enable_auto_help = false, const std::function<void(const std::map<std::string, std::list<std::string>>&)>& func = nullptr) -> CommandNode&;
		CommandExecuter(const CommandExecuter&) = delete;
		~CommandExecuter();
		void Execute(int argc,const char**argv);
		void SetOutputCallback(const std::function<void(const std::string& str)>& callback);
		
	private:
		std::map<std::string, std::list<std::string>> args;
		std::list<CommandNode*> nodes;
		std::function<void(const std::string& str)> output_callback=nullptr;
		CommandNode root;
	};
}
