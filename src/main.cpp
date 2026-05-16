#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>
#include <cstdlib>
#include <io.h>
#include <windows.h>
#include <filesystem>
#include <fstream>

struct ParsedCommand
{
	std::vector<std::string> arguments;
	bool isOutputRedi = false;
	bool isErrorRedi = false;
	std::string outputFile;
	std::string errorFile;
};

//std::map<std::string, std::function<void(std::vector<std::string>)>> builtins;
std::map<std::string, std::function<void(ParsedCommand)>> builtins;

std::vector<std::string> split_path(const std::string& path)
{
	std::vector<std::string> dirs;
	std::stringstream ss(path);
	std::string dir;

	while (std::getline(ss, dir, ';'))
	{
		if (!dir.empty())
		{
			dirs.push_back(dir);
		}
	}
	return dirs;
}

std::string build_command_line(const std::string& command, const std::vector<std::string>& args)
{
	std::string result = command;

	for (const auto& arg : args)
	{
		result += " ";

		if (arg.find(' ') != std::string::npos)
			result += "\"" + arg + "\"";
		else
			result += arg;
	}

	return result;
}

ParsedCommand parse_arguments(std::string arguments)
{
	ParsedCommand structAfterParsing;
	std::vector<std::string> parsedArguments;
	std::string currArgument;
	bool singleQuoteFlag = false;
	bool doubleQuoteFlag = false;
	for (size_t i = 0; i < arguments.length(); i++)
	{
		if (arguments[i] == '\\' && singleQuoteFlag==false)
		{
			currArgument += arguments[i+1];
			++i;
			continue;
		}
		if (arguments[i] == '\"')
		{
			doubleQuoteFlag = !doubleQuoteFlag;
			continue;
		}
		if (doubleQuoteFlag)
		{
			currArgument += arguments[i];
			continue;
		}
		if (arguments[i] == '\'')
		{
			singleQuoteFlag = !singleQuoteFlag;
			continue;
		}
		if (singleQuoteFlag)
		{
			currArgument += arguments[i];
			continue;
		}
		if (arguments[i] == ' ')
		{
			if (currArgument.empty())
			{
				continue;
			}
			if (currArgument == ">" || currArgument == "1>" || currArgument == "2>")
			{
				if (i + 1 >= arguments.length())
				{
					currArgument.erase();
					continue;
				}
				if (currArgument == "2>")
				{
					structAfterParsing.isErrorRedi = true;
					structAfterParsing.errorFile = arguments.substr(i + 1);
				}
				else
				{
					structAfterParsing.isOutputRedi = true;
					structAfterParsing.outputFile = arguments.substr(i + 1);
				}
				structAfterParsing.arguments = parsedArguments;
				return structAfterParsing;
			}
			parsedArguments.push_back(currArgument);
			currArgument = "";
			continue;
		}
		currArgument += arguments[i];
	}
	if (!currArgument.empty())
	{
		parsedArguments.push_back(currArgument);
	}
	structAfterParsing.arguments = parsedArguments;
	return structAfterParsing;
}

void echo(ParsedCommand arguments)
{
	for (auto argument : arguments.arguments)
	{
		std::cout << argument << " ";
	}
	std::cout << std::endl;
}

void shell_exit(ParsedCommand arguments)
{
	std::exit(0);
}

void shell_type(ParsedCommand arguments)
{
	if (arguments.arguments.empty())
	{
		std::cout << "type: missing argument\n";
		return;
	}

	if (builtins.count(arguments.arguments[0]))
	{
		std::cout << arguments.arguments[0] << " is a shell builtin" << std::endl;
	}
	else
	{
		const char* path_env = std::getenv("PATH");
		if (!path_env)
		{
			std::cout << arguments.arguments[0] << ": command not found" << std::endl;
		}
		else
		{
			std::vector<std::string> dirs = split_path(path_env);
			std::vector<std::string> extensions = { "", ".exe", ".bat", ".cmd" };

			for (const auto& dir : dirs)
			{
				for (const auto& ext: extensions)
				{
					std::string full_path = dir + "\\" + arguments.arguments[0] + ext;
					if (_access(full_path.c_str(), 0) == 0)
					{
						std::cout << arguments.arguments[0] << " is " << full_path << "\n";
						return;
					}
				}
			}
			std::cout << arguments.arguments[0] << ": command not found" << std::endl;
		}
	}
}

void shell_pwd(ParsedCommand arguments)
{
	std::cout << std::filesystem::current_path() << "\n";
}

void shell_cd(ParsedCommand arguments)
{
	if (arguments.arguments.empty())
	{
		std::cout << "No arguments passed\n";
	}
	if (arguments.arguments[0] == "~")
	{
		const char* home = std::getenv("USERPROFILE");
		if (home)
		{
			std::filesystem::current_path(home);
		}
		else
		{
			std::cout << "cd: " << arguments.arguments[0] << ": No such file or directory\n";
		}
	}
	else
	{
		if (std::filesystem::exists(arguments.arguments[0]) && std::filesystem::is_directory(arguments.arguments[0]))
		{
			std::filesystem::current_path(arguments.arguments[0]);
		}
		else
		{
			std::cout << "cd: " << arguments.arguments[0] << ": No such file or directory\n";
		}
	}
}

int main() {
	builtins.insert({ "echo", echo });
	builtins.insert({ "exit", shell_exit });
	builtins.insert({ "type", shell_type });
	builtins.insert({ "pwd", shell_pwd });
	builtins.insert({ "cd", shell_cd });
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
	while (true)
	{
		std::cout << ":3 ";
		std::string input; 
		std::getline(std::cin, input);
		if (!input.empty())
		{
			std::string command;
			std::string argumentsString;
			ParsedCommand arguments;
			for (size_t i = 0; i < input.size(); i++)
			{
				if (input[i] == ' ') 
				{
					command = input.substr(0, i);
					argumentsString = input.substr(i+1);
					break;
				}
			}
			if (command.length() == 0)
			{
				command = input;
			}
			arguments = parse_arguments(argumentsString);
			if (builtins.count(command))
			{
				if (arguments.isOutputRedi)
				{
					std::ofstream file(arguments.outputFile, std::ios::trunc);
					if (!file)
					{
						std::cerr << "Cannot open file: " << arguments.outputFile << "\n";
						continue;
					}
					auto* oldBuffer = std::cout.rdbuf(file.rdbuf());
					builtins[command](arguments);
					std::cout.rdbuf(oldBuffer);
				}
				else 
				{
					builtins[command](arguments);
				}
			}
			else
			{
				std::string commandLine = build_command_line(command, arguments.arguments);

				std::vector<char> buffer(commandLine.begin(), commandLine.end());
				buffer.push_back('\0');

				HANDLE stdoutHandle = NULL;

				if (arguments.isOutputRedi)
				{
					SECURITY_ATTRIBUTES sa{};
					sa.nLength = sizeof(sa);
					sa.lpSecurityDescriptor = NULL;
					sa.bInheritHandle = TRUE;

					stdoutHandle = CreateFileA(
						arguments.outputFile.c_str(),
						GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						&sa,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL
					);

					if (stdoutHandle == INVALID_HANDLE_VALUE)
					{
						std::cerr << "Cannot open file: " << arguments.outputFile << "\n";
						continue;
					}
				}

				STARTUPINFOA si{};
				si.cb = sizeof(si);

				if (arguments.isOutputRedi)
				{
					si.dwFlags |= STARTF_USESTDHANDLES;
					si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
					si.hStdOutput = stdoutHandle;
					si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
				}

				PROCESS_INFORMATION pi{};

				BOOL ok = CreateProcessA(
					NULL,
					buffer.data(),
					NULL,
					NULL,
					arguments.isOutputRedi ? TRUE : FALSE,
					0,
					NULL,
					NULL,
					&si,
					&pi
				);

				if (!ok)
				{
					DWORD err = GetLastError();

					if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
					{
						std::cout << command << ": command not found" << std::endl;
					}
					else
					{
						std::cout << "Error: " << err << "\n";
					}
				}
				else
				{
					WaitForSingleObject(pi.hProcess, INFINITE);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}

				if (arguments.isOutputRedi)
				{
					CloseHandle(stdoutHandle);
				}
			}
		}
	}
}