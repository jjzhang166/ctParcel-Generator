#pragma once

#include <windows.h>

/************************************************************************/
/* ��ȡ�ļ���С
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
/* ��ȡ·���е��ļ��� 
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
