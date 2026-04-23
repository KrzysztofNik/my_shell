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

std::map<std::string, std::function<void(std::vector<std::string>)>> builtins;

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

std::vector<std::string> parse_arguments(std::string arguments)
{
	std::vector<std::string> parsedArguments;
	std::string currArgument;
	for (size_t i = 0; i < arguments.length(); i++)
	{
		if (arguments[i] == ' ')
		{
			if (currArgument.empty())
			{
				continue;
			}
			parsedArguments.push_back(currArgument);
			currArgument = "";
			continue;
		}
		currArgument += arguments[i];
	}
	parsedArguments.push_back(currArgument);
	return parsedArguments;
}

void echo(std::vector<std::string> arguments)
{
	for (auto argument : arguments)
	{
		std::cout << argument << " ";
	}
	std::cout << std::endl;
}

void shell_exit(std::vector<std::string> arguments)
{
	std::exit(0);
}

void type(std::vector<std::string> arguments)
{
	if (builtins.count(arguments[0]))
	{
		std::cout << arguments[0] << " is a shell builtin" << std::endl;
	}
	else
	{
		const char* path_env = std::getenv("PATH");
		if (!path_env)
		{
			std::cout << arguments[0] << ": command not found" << std::endl;
		}
		else
		{
			std::vector<std::string> dirs = split_path(path_env);
			std::vector<std::string> extensions = { "", ".exe", ".bat", ".cmd" };

			for (const auto& dir : dirs)
			{
				for (const auto& ext: extensions)
				{
					std::string full_path = dir + "\\" + arguments[0] + ext;
					if (_access(full_path.c_str(), 0) == 0)
					{
						std::cout << arguments[0] << " is " << full_path << "\n";
						return;
					}
				}
			}
			std::cout << arguments[0] << ": command not found" << std::endl;
		}
	}
}

void shell_pwd(std::vector<std::string> arguments)
{
	std::cout << std::filesystem::current_path() << "\n";
}

void shell_cd(std::vector<std::string> arguments)
{
	if (arguments[0] == "~")
	{
		const char* home = std::getenv("USERPROFILE");
		if (home)
		{
			std::filesystem::current_path(home);
		}
		else
		{
			std::cout << "cd: " << arguments[0] << ": No such file or directory\n";
		}
	}
	else
	{
		if (std::filesystem::exists(arguments[0]) && std::filesystem::is_directory(arguments[0]))
		{
			std::filesystem::current_path(arguments[0]);
		}
		else
		{
			std::cout << "cd: " << arguments[0] << ": No such file or directory\n";
		}
	}
}

int main() {
	builtins.insert({ "echo", echo });
	builtins.insert({ "exit", shell_exit });
	builtins.insert({ "type", type });
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
			std::vector<std::string> arguments;
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
				builtins[command](arguments);
			}
			else
			{
				std::vector<char> buffer(input.begin(), input.end());
				buffer.push_back('\0');
				STARTUPINFOA si{};
				si.cb = sizeof(si);
				PROCESS_INFORMATION pi{};
				if (!CreateProcessA(
					NULL,
					buffer.data(),
					NULL,
					NULL,
					FALSE,
					0,
					NULL,
					NULL,
					&si,
					&pi
				))
				{
					DWORD err = GetLastError();
					if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
					{
						std::cout << command << ": command not found" << std::endl;
					}
					else {
						std::cout << "Error: " << err << "\n";
					}
				}
				else
				{
					WaitForSingleObject(pi.hProcess, INFINITE);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
			}
		}
	}
}