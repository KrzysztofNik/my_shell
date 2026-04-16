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

std::map<std::string, std::function<void(std::string)>> builtins;

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

void echo(std::string argument)
{
	std::cout << argument << std::endl;
}

void shell_exit(std::string argument)
{
	std::exit(0);
}

void type(std::string argument)
{
	if (builtins.count(argument))
	{
		std::cout << argument << " is a shell builtin" << std::endl;
	}
	else
	{
		const char* path_env = std::getenv("PATH");
		if (!path_env)
		{
			std::cout << argument << ": command not found" << std::endl;
		}
		else
		{
			std::vector<std::string> dirs = split_path(path_env);
			std::vector<std::string> extensions = { "", ".exe", ".bat", ".cmd" };

			for (const auto& dir : dirs)
			{
				for (const auto& ext: extensions)
				{
					std::string full_path = dir + "\\" + argument + ext;
					if (_access(full_path.c_str(), 0) == 0)
					{
						std::cout << argument << " is " << full_path << "\n";
						return;
					}
				}
			}
			std::cout << argument << ": command not found" << std::endl;
		}
	}
}

void shell_pwd(std::string argument)
{
	std::cout << std::filesystem::current_path() << "\n";
}

void shell_cd(std::string argument)
{

}

int main() {
	builtins.insert({ "echo", echo });
	builtins.insert({ "exit", shell_exit });
	builtins.insert({ "type", type });
	builtins.insert({ "pwd", shell_pwd });
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
	while (true)
	{
		std::cout << "$ ";
		std::string input;
		std::getline(std::cin, input);
		if (!input.empty())
		{
			std::string command;
			std::string arguments;
			for (size_t i = 0; i < input.size(); i++)
			{
				if (input[i] == ' ') 
				{
					command = input.substr(0, i);
					arguments = input.substr(i+1);
					break;
				}
			}
			if (command.length() == 0)
			{
				command = input;
			}

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