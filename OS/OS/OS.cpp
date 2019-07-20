#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>
#include <list>
#include <regex>
#include <thread>

using namespace std;

struct Parameter {
	string path;
	regex reg;
};

HANDLE mut = CreateMutex(NULL, FALSE, "fileList");
vector<string> fileList;

list<string> GetDisks()
{
	list<string> result;
	char szDrive[] = "A:";
	DWORD uDriveMask = GetLogicalDrives();
	if (uDriveMask == 0)
	{
		printf("GetLogicalDrives() failed with failure code: %d\n", GetLastError());
	}
	else
	{
		printf("This machine has the following logical drives:\n");
		while (uDriveMask)
		{
			if (uDriveMask & 1)
			{
				UINT dtype = GetDriveType(szDrive);
				if (dtype == DRIVE_REMOVABLE || dtype == DRIVE_FIXED || dtype == DRIVE_REMOTE)
				{
					cout << szDrive;
					cout << " ";
					result.push_back(szDrive);
				}
			}
			++szDrive[0];
			uDriveMask >>= 1;
		}
		printf("\n");
	}
	return result;
}

void AppendFile(const string& directory, const string& fileName, const regex& reg)
{
	if (!regex_match(fileName, reg)) {
		return;
	}

	DWORD result;
	result = WaitForSingleObject(mut, INFINITE);
	if (result == WAIT_OBJECT_0)
	{
		fileList.push_back(directory + fileName + "\r\n");
		ReleaseMutex(mut);
	}
	else
	{
		cout << "Mutex failed!" << endl;
	}
}

void GetFiles(string directory, const regex & reg, bool recursively = true)
{
	string filePath;

	directory += "\\";
	string filter = directory + "*";

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE) return;

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (recursively && (lstrcmp(FindFileData.cFileName, TEXT(".")) != 0) && (lstrcmp(FindFileData.cFileName, TEXT("..")) != 0))
			{
				GetFiles(directory + FindFileData.cFileName, reg);
			}
		}
		else
		{
			AppendFile(directory, FindFileData.cFileName, reg);
		}
	} while (FindNextFile(hFind, &FindFileData) != 0);

	FindClose(hFind);
}

DWORD WINAPI GetFilesAsync(LPVOID lpParam)
{
	Parameter* parameter = (Parameter*)lpParam;
	GetFiles(parameter->path, parameter->reg);

	return 0;
}

int _tmain(int argc, char* argv[])
{
	regex reg(".*(xls|docx|xlsx)");

	list<string> disks = GetDisks();

	vector<HANDLE> threads;
	for (int i = 0; i < disks.size(); i++)
	{
		Parameter* data = new Parameter;
		data->path = disks.back();
		data->reg = reg;

		HANDLE handle = CreateThread(NULL, 0, GetFilesAsync, data, 0, NULL);
		threads.push_back(handle);
	}

	WaitForMultipleObjects(threads.size(), threads.data(), TRUE, INFINITE);

	for (int i = 0; i < threads.size(); i++) {
		CloseHandle(threads[i]);
	}
	CloseHandle(mut);


	HANDLE logFile = CreateFile("logs.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (logFile != INVALID_HANDLE_VALUE)
	{
		DWORD bytesWritten;

		for (int i = 0; i < fileList.size(); i++)
		{
			cout << fileList[i].c_str();
			WriteFile(logFile, fileList[i].c_str(), fileList[i].length(), &bytesWritten, NULL);
		}
		CloseHandle(logFile);
	}
	else
	{
		cout << "Error write logs!" << endl;
	}

	return 0;
}