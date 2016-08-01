#pragma once

#include <windows.h>

/************************************************************************/
/* 获取文件大小
/************************************************************************/
int FsGetFileSize( char * filename )
{
	HANDLE hfile = CreateFileA( filename, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL );
	if(hfile)
	{
		int size = GetFileSize( hfile, NULL );
		CloseHandle( hfile );
		return size;
	}

	return 0;
}

/************************************************************************/
/* 获取路径中的文件名 
/************************************************************************/
char* FsGetFileName(char* path)
{
	int len = strlen( path );
	for(int i = len; i > 0; i--)
	{
		if(path[i] == '\\')
			return path + i + 1;
	}
	return path;
}
