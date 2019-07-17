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

list<string> fileList;

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
				if (dtype == DRIVE_REMOVABLE || dtype == DRIVE_FIXED || dtype == DRIVE_REMOTE) //выводит только нормальые диски (видимые)
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

	fileList.push_back(directory + fileName);
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

int _tmain(int argc, char* argv[])
{
	regex reg(".*(xls|docx|xlsx)");

	list<string> disks = GetDisks();

	vector<thread> threads;
	for (const auto& disk : disks)
	{
		cout << disk << endl;
		threads.push_back(thread(GetFiles, disks.back(), reg, true));
	}
	for (int i = 0; i < threads.size(); i++) {
		threads[i].join();
	}

	ofstream myfile("logs.txt");
	if (myfile.is_open())
	{
		for (list<string>::iterator it = fileList.begin(); it != fileList.end(); ++it)
		{
			cout << (*it) << endl;
			myfile << (*it) << endl;
		}
		myfile.close();
	}
	else cout << "Unable to open file";

	return 0;
}

