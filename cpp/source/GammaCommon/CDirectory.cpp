#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#ifdef WIN32
#pragma warning (disable:4800)
#include <winsock.h>
#include <io.h>
#endif
#include "GammaCommon/CDirectory.h"
#include <iostream>
/*
		class CDir
*/
CDirectory::CDirectory()
{
	this->bOpened = false;
	this->bNext = false;
	strDirectory.clear();
	strFileName.clear();
	memset(&fp, 0, sizeof(fp));
#ifdef WIN32
	dp = INVALID_HANDLE_VALUE;
#else
	dp = NULL;
#endif
}

CDirectory::~CDirectory()
{
	Close();
}

bool CDirectory::OpenDirectory(const char* pathname)
{
	this->strDirectory = pathname;
	if ( strDirectory[strDirectory.length()-1] != '\\' &&
		strDirectory[strDirectory.length()-1] != '/' )
	{
#ifdef WIN32
		strDirectory += "\\";
#else
		strDirectory += "/";
#endif
	}
#ifdef WIN32
	dp = FindFirstFile((strDirectory + "*").c_str(),&this->fp);
	if ( dp == INVALID_HANDLE_VALUE )
		return false;
#else
	dp = opendir(strDirectory.c_str());
	if ( dp == NULL )
		return false;
#endif
	this->bOpened = true;
	return true;
}

bool CDirectory::HasNextFile()
{
	if ( !bOpened )
		return false;
	this->strFileName = "";
#ifdef WIN32
	this->bNext = FindNextFile(dp,&fp);
	if ( bNext )
		strFileName = fp.cFileName;
#else
	struct dirent *dirp;
	dirp = readdir(dp);
	if ( dirp == NULL )
		bNext = false;
	else
	{
		bNext =true;
		strFileName = dirp->d_name;
		char filename[500];
		sprintf(filename,"%s%s",strDirectory.c_str(),strFileName.c_str());
		stat(filename,&fp);
	}
#endif
	return bNext;
}

const char* CDirectory::NextFile()
{
	return this->strFileName.c_str();
}

const char* CDirectory::GetDirectory()
{
	return this->strDirectory.c_str();
}

bool CDirectory::IsDirectory()
{
#ifdef WIN32
	return ( fp.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
#else
	return ( S_ISDIR(fp.st_mode) );
#endif
}

bool CDirectory::IsFile()
{
#ifdef WIN32
	return !( fp.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
#else
	return ( S_ISREG(fp.st_mode) );
#endif
}

ulong64 CDirectory::GetFileSize()
{
	ulong64 filesize;
	filesize.HighFileSize = 0;
	filesize.LowFileSize = 0;
#ifdef WIN32
	filesize.HighFileSize = fp.nFileSizeHigh;
	filesize.LowFileSize = fp.nFileSizeLow;
#else
	filesize.LowFileSize = fp.st_size & 0xFFFFFFFF;
#ifdef _64BIT_
	filesize.HighFileSize = fp.st_size >> 8 ;
#endif
#endif
	return filesize;
}

int CDirectory::GetNodeType(const char* pathname)
{
	STAT_DATA statbuf;
#ifdef WIN32
	DIR_HANDLE fHandle;
	fHandle = FindFirstFile(pathname, &statbuf);
	if(fHandle == INVALID_HANDLE_VALUE)
		return DIR_INEXISTENCE;
	if(statbuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return DIR_DIRECTORY;
	else
		return DIR_FILE;
#else
	if(lstat(pathname, &statbuf) == -1)
		return DIR_INEXISTENCE;
	if(S_ISDIR(statbuf.st_mode))
		return DIR_DIRECTORY;
	else
		return DIR_FILE;
#endif
}

//递归创建目录
void CDirectory::MakeDirectory(const char *pathname)
{
	const char *p = pathname;
	char pathBuf[1024] = {0};
#ifdef WIN32
	char separator = '\\';
#else
	char separator = '/';
#endif
	for(p = strchr(p, separator); p != NULL; p = strchr(p, separator))
	{
		strncpy(pathBuf, pathname, p-pathname);
#ifdef WIN32
		_mkdir(pathBuf);
#else
		mkdir(pathBuf, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
		++p;
	}
#ifdef WIN32
	_mkdir(pathname);
#else
	mkdir(pathname, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
}

void CDirectory::Close()
{
#ifdef WIN32
	if ( dp != INVALID_HANDLE_VALUE )
		FindClose(dp);
	dp = INVALID_HANDLE_VALUE;
#else
	if ( dp != NULL )
		closedir(dp);
	dp = NULL;
#endif
	bOpened = false;
}

/*
		class CFileFilter
		?表示任意一个字节
		*表示任意长度的字节
*/
CFileFilter::~CFileFilter()
{
}

void CFileFilter::SetFilter(const char* filter)
{
	filematchname = filter;
}

bool CFileFilter::IsMatch(const char* filename)
{
	int oldstate = -1;
	int iFilterBeginIndex = 0,iFileNameIndex =0;
	char curch=0,prevch=0,firstch = 0;
	for ( iFilterBeginIndex = 0 ; iFilterBeginIndex < (int)filematchname.length(); iFilterBeginIndex++)
	{
		if ( iFileNameIndex >= strlen(filename) )
			return false;
		curch = filematchname[iFilterBeginIndex];
		switch ( curch )
		{
			case '?' :
				iFileNameIndex++;
				break;
			case '*' :
				oldstate = iFilterBeginIndex;
				break;
			default :
				if ( prevch == '*' || prevch == 0 )
					firstch = curch;
				if ( curch != filename[iFileNameIndex] )
				{
					if ( oldstate == -1 )
						return false;
					else
					{
						if ( firstch == filename[iFileNameIndex] )
							iFileNameIndex --;
						iFilterBeginIndex = oldstate;
					}
				}
				iFileNameIndex++;
		}
		prevch = curch;
	}
	//出现文件名还有剩余长度的情况
	if ( iFileNameIndex != strlen(filename))
	{
		//如果后面是*的匹配则认为完全正确
		if ( curch == '*' )
			return true;
		//如果匹配规则中曾经出现*号则进行反向匹配
		else if ( oldstate != -1 )
		{
			iFileNameIndex = (int)strlen(filename) - 1;
			//从文件名后进行匹配
			for ( iFilterBeginIndex--;iFilterBeginIndex >= 0;iFilterBeginIndex--,iFileNameIndex-- )
			{
				curch = filematchname[iFilterBeginIndex];
				switch ( curch )
				{
					case '?' :
						break;
					case '*' :
						return true;
					default :
						if ( curch != filename[iFileNameIndex] )
							return false;
				}
			}
			return false;
		}
		//否则认为匹配错误
		else
			return false;
	}
	return true;
}

