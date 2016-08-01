// generator.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include <stdio.h>
#include <windows.h>
#include <list>
#include <string>
#include "common.h"
#include "zip.h"
using namespace std;

#define ISDEBUG   0

char SetupFilePath[260] = {0};			    //"D:\\code\\ctparcel\\setup\\Debug\\ctparcel.exe"
char ProgramName[100] = {0};				//"xxx��װ��"
char FolderName[260] = {0};				    //"e:\\1"
char compressStr[10] = {0};                 //"zip"
char AutoRunName[150] = {0};                //"autorun.exe"

//////////////////////////////////////////////////////////////////////////
// �����ɵ�����exe���ļ��������һ���ṹ,����ṹ�����˱����UIͼƬ/����/ѹ���ļ���
struct fileblock
{
    char relativepath[150];			// ��ʱ�ļ�=".tmp" or ���·�� : aa/bb/
    char filename[50];				// �ļ���   : a.dll   >> ��ô����ļ���·��: setuppath/aa/bb/a.dll
    DWORD filesize;					// ��С
                                    //
                                    // +sizeof(fileblock) ֮������ļ����� byte* filebuf
};
#define RELATIVEPATH_TMP	".tmp"

struct addedSector
{
    DWORD verifycode;               // 0

    char compressType[10];			// ѹ����ʽ
    char programName[20];			// ��װ���ĳ�����
    char autorun[150];              // �Զ����г�������·��

    int nfiles;						// ���ļ�����
                                    //
                                    // +sizeof(addedSector) ֮�����ÿ���ļ��� fileblock
};


//
// open file and write to end of file
bool appendFile( const char* filename, byte* buf, DWORD size )
{
    FILE * f = fopen(filename, "ab+" );
    if(f)
    {
        fwrite( buf, size, 1, f );
        fclose( f );
        return true;
    }
    else
    {
        //Ϊ�˷�ֹ���̵�cache���µ�fclose������ˢ���ļ�
        //����һֱ����ȥ���ļ�
        Sleep( 1 );
        //printf("fopen err: %d\n",GetLastError());
        return appendFile( filename, buf, size );
    }
    
    return false;
}


//
// ѹ�������ļ� [�ļ�ֱ�ӱ�ѹ�����滻��ԭʼ�ļ�]
//
void compressSingleFile(char* filename)
{
    if(strcmp( compressStr, "zip" ) == 0)
    {
        char tmpfile[260];
        strcpy( tmpfile, filename );
        strcat( tmpfile, ".zip" );

        HZIP hz = CreateZip( tmpfile, 0 );
        ZipAdd( hz, FsGetFileName( filename ), filename );
        CloseZip( hz );

        DeleteFileA( filename );
        MoveFileA( tmpfile, filename );
    }
}

//
// ����ļ�
//
int appendfilecount = 0;
bool addFile( char* orifilename, char* relativepath )
{
    // -> copy file to temp path
    // -> optional : compress file
    // -> get file-size
    // -> ns = new sizeof(fileblock) + file-size
    // -> init fileblock struct
    // -> read file to ->  ns + sizeof(fileblock)
    // -> add ns to end of setup-file
    // -> delete temp file
    char* shortFileName = FsGetFileName( orifilename );

    char filename[260];
    GetTempPathA( 260, filename );
    strcat( filename, shortFileName );
    DeleteFileA( filename );
    CopyFileA( orifilename, filename, FALSE );
    
    if(compressStr[0])
        compressSingleFile( filename );

    int fsize = FsGetFileSize( filename );
    if(fsize)
    {
        int allsize = sizeof( fileblock ) + fsize;

        fileblock* ns = (fileblock*)(new byte[allsize]);
        memset( ns, 0, allsize );
        strcpy( ns->relativepath, relativepath );
        ns->filesize = fsize;
        strcpy( ns->filename, shortFileName );
        PBYTE filebuf = (PBYTE)((DWORD)ns + sizeof( fileblock ));

        FILE* f = fopen( filename, "rb+" );
        if(f)
        {
            fread( filebuf, fsize, 1, f );
            fclose( f );

            if(appendFile( SetupFilePath, (PBYTE)ns, allsize ))
                printf( "[%d] addFile ok =>  %s\\%s\n", ++appendfilecount, ns->relativepath, ns->filename );
            else
                printf( "[%d] addFile error!!! =>  %s\\%s\n", ++appendfilecount, ns->relativepath, ns->filename );
        }
    }

    DeleteFileA( filename );
    return true;
}



//
// ����ļ����ڵ������ļ�
//
// ȫ��:
list<string> allfiles;


// ��ʽ: e:\\1
char rootdirpath[260] = {0};
// ��ʽ: 2\\3
char relativeTmpPath[260];
//
char* getRelativePath( char* allPath )
{
    relativeTmpPath[0] = 0;
    PCH tmp = strstr( allPath, rootdirpath );
    if(tmp)
    {
        int i = strlen( allPath );
        for(; i > 0; i--)
        {
            if(allPath[i] == '\\')
            {
                allPath[i] = 0;
                break;
            }
        }
        if(*(tmp + strlen( rootdirpath )) != 0)
            strcpy( relativeTmpPath, tmp + strlen( rootdirpath ) + 1 );
        allPath[i] = '\\';
    }
    return relativeTmpPath;
}
//
void addFolder()
{
    int itmp=0;
    for(auto &file : allfiles)
    {
        //printf( "-------> [%d] %s\n", ++itmp, (char*)file.c_str() );
        addFile( (char*)file.c_str(), getRelativePath( (char*)file.c_str() ) );
    }
}
//
bool _initFolder( char* lpPath )
{
    char szFind[MAX_PATH];
    WIN32_FIND_DATAA FindFileData = {0};
    strcpy( szFind, lpPath );
    strcat( szFind, "\\*.*" );
    HANDLE hFind = FindFirstFileA( szFind, &FindFileData );
    if(INVALID_HANDLE_VALUE == hFind)
        return false;

    do
    {
        if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if(strcmp( FindFileData.cFileName, "." ) != 0 &&
                strcmp( FindFileData.cFileName, ".." ) != 0)
            {
                //������Ŀ¼���ݹ�֮
                char szFile[MAX_PATH] = {0};
                strcpy( szFile, lpPath );
                strcat( szFile, "\\" );
                strcat( szFile, FindFileData.cFileName );
                _initFolder( szFile );
            }
        }
        else
        {
            //�ų�����ļ�
            if(strcmp( FindFileData.cFileName, "Thumbs.db" ) != 0)
            {
                //�������ļ���path����list
                char allPath[260];
                wsprintfA( allPath, "%s\\%s", lpPath, FindFileData.cFileName );
                allfiles.push_back( allPath );
            }
        }
    }
    while(FindNextFileA( hFind, &FindFileData ));

    FindClose( hFind );
    return true;
}
//
bool initFolder( char* rootdir )
{
    strcpy( rootdirpath, rootdir );
    return _initFolder( rootdirpath );
}


//
// ��ӱ�Ҫ�ṹ
//
bool addLastSector( int filescount, char* programName, char* compress,char* autorun )
{
    addedSector as = {0};
    as.nfiles = filescount;
    strcpy( as.programName, programName );
    strcpy( as.compressType, compress );
    strcpy( as.autorun, autorun );
    appendFile( SetupFilePath, (PBYTE)&as, sizeof( as ) );
    return true;
}


//
// ��ʼ����
//
void generateSetupFile()
{
    // ->
    // start!
    initFolder( FolderName );
    // add addedSector to end of setup-file 
    addLastSector( allfiles.size() + 4, ProgramName, compressStr, AutoRunName );
    // ->
    // add every file to end of setup-file
    //
    // setup tmp files
#if ISDEBUG
    addFile( "e:\\str.txt", RELATIVEPATH_TMP );
    addFile( "e:\\WizardImage.bmp", RELATIVEPATH_TMP );
    addFile( "e:\\WizardSmallImage.bmp", RELATIVEPATH_TMP );
    addFile( "e:\\folders.bmp", RELATIVEPATH_TMP );
#else
    //�Ȱѿյİ�װexe����������Ŀ����ȥ,Ȼ��ȥ����ļ�������
    CopyFileA( "setuptmp.exe", SetupFilePath, FALSE );
    addFile( "str.txt", RELATIVEPATH_TMP );
    addFile( "WizardImage.bmp", RELATIVEPATH_TMP );
    addFile( "WizardSmallImage.bmp", RELATIVEPATH_TMP );
    addFile( "folders.bmp", RELATIVEPATH_TMP );
#endif
    // ->
    // other files
    addFolder( );
}


//////////////////////////////////////////////////////////////////////////
int main( int argc, char** argv )
{
#if ISDEBUG
    strcpy( SetupFilePath, "D:\\����\\ctparcel\\setup\\Release\\ctparcel.exe" );
    strcpy( ProgramName, "Test��Ϸ" );
    strcpy( FolderName, "e:\\1" );
    strcpy( compressStr, "zip" );
    strcpy( AutoRunName, "update.exe" );
    generateSetupFile();
    return 0;
#endif

    //-------------------------------------------------------------------------------
    //					   ����Ŀ��		  ��װ������	    �ļ���    ѹ��     �Զ�����
    //
    // arg -> generator   -c:\setup.exe  -programName  -folder   -zip    -xxxx.exe
    //                                                           -none
    //-------------------------------------------------------------------------------

    bool bCorrectCommand = false;

    if(argc >= 4)
    {
        bCorrectCommand = true;

        strcpy( SetupFilePath, argv[1] + 1 );
        strcpy( ProgramName, argv[2] + 1 );
        strcpy( FolderName, argv[3] + 1 );
    }
    if(argc >= 5)
    {
        strcpy( compressStr, argv[4] + 1 );
    }
    if(argc >= 6)
    {
        strcpy( AutoRunName, argv[5] + 1 );
    }

    if( bCorrectCommand )
    {
        generateSetupFile();
        return 0;
    }

    puts( "����Ĳ���" );
    return 0;
}

