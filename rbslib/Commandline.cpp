#include "Commandline.h"
#include <exception>
#include <stdexcept>

using namespace RbsLib::Command;

bool CommandLine::Parse(const std::string& command)
{
	this->Clear();
	std::string arg;
	bool is_in = false;//标识是否在有效解析部分中
	bool q = false;//标识是否在双引号引起的部分中

	for (auto it : command)
	{
		if (q == true)
		{
			//当前在引号中
			if (it == '\"')
			{
				//离开引号区域
				q = false;
			}
			else arg.push_back(it);
		}
		else
		{
			//当前不在引号中
			if (it == ' ')
			{
				if (is_in == true)
				{
					//终止有效部分解析
					is_in = false;
					if (arg != "") args.push_back(arg);
					arg.clear();
				}
			}
			else
			{
				//进入有效解析区域
				is_in = true;
				if (it == '\"') q = true;//进入引号区域
				else arg.push_back(it);
			}
		}
	}
	if (!arg.empty()) args.push_back(arg);
	return true;
}

CommandLine::CommandLine(int argc, const char** argv)
{
	if (argc > 0)
	{
		for (int i = 0; i < argc; ++i)
		{
			this->args.push_back(argv[i]);
		}
	}
}

bool CommandLine::IsFlagExist(const std::string& flag)
{
	if (std::find(this->args.begin(), this->args.end(), flag) != this->args.end())
	{
		return true;
	}
	return false;
}

std::string CommandLine::GetNextToken(const std::string& flag) const
{
	auto it = std::find(this->args.begin(), this->args.end(), flag);
	if (it != this->args.end() && (++it) != args.end())
	{
		return *it;
	}
	return std::string();
}

std::string CommandLine::operator[](int index) const
{
	return this->args.size() > index ? this->args[index] : std::string();
}

std::string CommandLine::operator[](const std::string& flag) const
{
	return this->GetNextToken(flag);
}

int CommandLine::GetSize() const
{
	return this->args.size();
}

void CommandLine::Clear()
{
	this->args.clear();
}

void CommandLine::Push(const std::string& arg)
{
	this->args.push_back(arg);
}



auto RbsLib::Command::CommandExecuter::operator[](const std::string& str) -> CommandNode&
{
	return root[str];
}

CommandExecuter::CommandExecuter()
	:root(&this->nodes, &this->output_callback)
{
}

auto RbsLib::Command::CommandExecuter::CreateSubOption(const std::string& option, int have_extra_arg, const std::string description, bool is_enable_auto_help, const std::function<void(const std::map<std::string, std::list<std::string>>&)>& func) -> CommandNode&
{
	return root.CreateSubOption(option, have_extra_arg, description, is_enable_auto_help, func);
}

RbsLib::Command::CommandExecuter::~CommandExecuter()
{
	for (auto it : this->nodes)
	{
		delete it;
	}
}

void RbsLib::Command::CommandExecuter::Execute(int argc, const char** argv)
{
	try
	{
		CommandNode* ptr = &this->root;
		bool is_need_run = true;
		for (int i = 0; i < argc; ++i)
		{
			if (int extra_option_num = ptr->OptionNum(argv[i]))
			{
				if (extra_option_num == -2) is_need_run = false;
				if (extra_option_num > 0)
				{
					ptr = &(*ptr)[argv[i]];
					std::string option_name = argv[i];
					if (i + extra_option_num >= argc) throw std::runtime_error("Missing parameters");
					for (int j = 0; j < extra_option_num; ++j)
					{
						this->args[option_name].push_back(argv[++i]);
					}
				}
				else if (extra_option_num == -1)
				{
					ptr = &(*ptr)[argv[i]];
					std::string option_name = argv[i];
					while (1)
					{
						try
						{
							if (++i >= argc) break;
							if (-2 == (*ptr).OptionNum(argv[i]))
							{
								if (i + 1 != argc) throw 1;
								(*ptr)[argv[i]];
								is_need_run = false;
								++i;
								break;
							}
							--i;
							break;
						}
						catch (const std::runtime_error&)
						{
							this->args[option_name].push_back(argv[i]);
						}
					}
				}
				else if (extra_option_num == -2)
				{
					if (i + 1 != argc) throw 1;
					(*ptr)[argv[i]];
					break;
				}
			}
			else
			{
				ptr = &(*ptr)[argv[i]];
			}

		}
		if (is_need_run)
		{
			if (ptr->IsFunctionExist())
				ptr->operator()(this->args);
			else throw std::runtime_error("语法错误，使用-h参数获取帮助");
		}
	}
	catch (int)
	{
		throw std::runtime_error("语法错误，使用-h参数获取帮助");
	}
}

void RbsLib::Command::CommandExecuter::SetOutputCallback(const std::function<void(const std::string& str)>& callback)
{
	this->output_callback = callback;
}

auto RbsLib::Command::CommandExecuter::CommandNode::operator[](const std::string& str)-> CommandNode&
{
	if (this->children.find(str) != this->children.end())
	{
		return *std::get<1>(this->children[str]);
	}
	else
	{
		//检查当前选项是否是--help或-h如果是则输出帮助信息
		if ((str == "--help" || str == "-h") && this->enable_auto_help)
		{
			(*(this->output_callback))("Options:");
			for (const auto& it : this->children)
			{
				(*(this->output_callback))(it.first + " : " + std::get<2>(it.second));
			}
			return *this;
		}
		else throw std::runtime_error(std::string("No such option: ") + str);
	}
}

void RbsLib::Command::CommandExecuter::CommandNode::operator()(const std::map<std::string, std::list<std::string>>& args)
{
	this->func(args);
}

void RbsLib::Command::CommandExecuter::CommandNode::SetFunction(const std::function<void(const std::map<std::string, std::list<std::string>>&)>& func)
{
	this->func = func;
}

auto CommandExecuter::CommandNode::CreateSubOption(const std::string& option, int have_extra_arg, const std::string description, bool is_enable_auto_help, const std::function<void(const std::map<std::string, std::list<std::string>>&)>& func) -> CommandNode&
{
	if (have_extra_arg < -1) throw std::runtime_error("Extra args number less than -1");
	auto x = new CommandNode(this->nodes, this->output_callback);
	this->nodes->push_back(x);
	x->func = func;
	this->children[option] = { have_extra_arg,x,description };
	x->enable_auto_help = is_enable_auto_help;
	return *x;
}

auto RbsLib::Command::CommandExecuter::CommandNode::CreateSelfReferenceOption(const std::string& option, int extra_args_num, const std::string& description) -> CommandNode&
{
	if (extra_args_num < -1) throw std::runtime_error("Extra args number less than -1");
	this->children[option] = { extra_args_num,this,description };
	return *this;
}

RbsLib::Command::CommandExecuter::CommandNode::CommandNode(std::list<CommandNode*>* nodes, const std::function<void(const std::string& str)>* callback)
	:nodes(nodes), output_callback(callback)
{
}

int RbsLib::Command::CommandExecuter::CommandNode::OptionNum(const std::string& option)
{
	if (this->children.find(option) == this->children.end())
	{
		if (this->enable_auto_help && (option == "-h" || option == "--help")) return -2;
		else
			throw std::runtime_error(std::string("No such option: ") + option);
	}
	return std::get<0>(this->children[option]);
}

bool RbsLib::Command::CommandExecuter::CommandNode::IsFunctionExist() const noexcept
{
	return static_cast<bool>(this->func);
}
