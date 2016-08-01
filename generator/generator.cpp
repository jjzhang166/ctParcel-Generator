// generator.cpp : 定义控制台应用程序的入口点。
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
char ProgramName[100] = {0};				//"xxx安装包"
char FolderName[260] = {0};				    //"e:\\1"
char compressStr[10] = {0};                 //"zip"
char AutoRunName[150] = {0};                //"autorun.exe"

//////////////////////////////////////////////////////////////////////////
// 在生成的最终exe的文件后面添加一个结构,这个结构包含了必须的UI图片/配置/压缩文件等
struct fileblock
{
    char relativepath[150];			// 临时文件=".tmp" or 相对路径 : aa/bb/
    char filename[50];				// 文件名   : a.dll   >> 那么这个文件的路径: setuppath/aa/bb/a.dll
    DWORD filesize;					// 大小
                                    //
                                    // +sizeof(fileblock) 之后就是文件内容 byte* filebuf
};
#define RELATIVEPATH_TMP	".tmp"

struct addedSector
{
    DWORD verifycode;               // 0

    char compressType[10];			// 压缩方式
    char programName[20];			// 安装包的程序名
    char autorun[150];              // 自动运行程序的相对路径

    int nfiles;						// 总文件数量
                                    //
                                    // +sizeof(addedSector) 之后就是每个文件块 fileblock
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
        //为了防止磁盘的cache导致的fclose后不立即刷新文件
        //这里一直尝试去打开文件
        Sleep( 1 );
        //printf("fopen err: %d\n",GetLastError());
        return appendFile( filename, buf, size );
    }
    
    return false;
}


//
// 压缩单个文件 [文件直接被压缩并替换掉原始文件]
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
// 添加文件
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
// 添加文件夹内的所有文件
//
// 全局:
list<string> allfiles;


// 格式: e:\\1
char rootdirpath[260] = {0};
// 格式: 2\\3
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
                //发现子目录，递归之
                char szFile[MAX_PATH] = {0};
                strcpy( szFile, lpPath );
                strcat( szFile, "\\" );
                strcat( szFile, FindFileData.cFileName );
                _initFolder( szFile );
            }
        }
        else
        {
            //排除这个文件
            if(strcmp( FindFileData.cFileName, "Thumbs.db" ) != 0)
            {
                //将所有文件的path放入list
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
// 添加必要结构
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
// 开始生成
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
    //先把空的安装exe拷贝到生成目标里去,然后去添加文件和配置
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
    strcpy( SetupFilePath, "D:\\代码\\ctparcel\\setup\\Release\\ctparcel.exe" );
    strcpy( ProgramName, "Test游戏" );
    strcpy( FolderName, "e:\\1" );
    strcpy( compressStr, "zip" );
    strcpy( AutoRunName, "update.exe" );
    generateSetupFile();
    return 0;
#endif

    //-------------------------------------------------------------------------------
    //					   生成目标		  安装包主名	    文件夹    压缩     自动运行
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

    puts( "错误的参数" );
    return 0;
}

