/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application, but
 * could be used for other applications which could use the PoolFile
 * class's functionality.
 * 
 * PoolFile is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * PoolFile is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include "CMultiFile.h"

#include <limits.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <stdio.h> // for rename

#include <stdexcept>
#include <string>
#include <algorithm>

#include <CPath.h>

//#if defined(__SOLARIS_GNU_CPP__) || defined(__SOLARIS_SUN_CPP__) || defined(__LINUX_GNU_CPP__)
	// ??? if windows is posix... I think it should have these necessary files... We'll see when we try to compile there
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>

/*
#elif defined(WIN32)
	#include <sys\stat.h>
	#include <share.h>
	#include <io.h>
	#include <stddef.h>
	#include <dos.h>

#else
	#error implementation not handled
#endif
*/

#define PHYSICAL_MAX_FILE_SIZE ((CMultiFile::l_addr_t)LONG_MAX) // ??? this needs to change based on off_t's type... won't be a problem when simple 64bit fs is in place commonly
#define LOGICAL_MAX_FILE_SIZE ((CMultiFile::l_addr_t)(PHYSICAL_MAX_FILE_SIZE-sizeof(CMultiFile::RFileHeader)))

#define CMULTIFILE_SIGNATURE (*((uint32_t *)"CMFL"))



/* -- CMultiFile --  
 *
 * - This is the file I/O class which (mainly) TPoolFile uses to read and write to disk
 *   	- When 64 bit file systems are the norm, I should be able to reimplement this 
 *   	  class for TPoolFile that just calls the normal file system functions
 *
 * - This class maps a larger address space onto multiple files
 *
 * - When 64 bit file systems become the norm this class should really be obsolete (for a
 *   time).
 *
 * - One practical consideration is that to provide full 64bit address access by using 
 *   multiple files, it would require up to 8 billion 31bit files to actually access a 64 
 *   bit sized space.   My purpose was not necessarily to provide 18Tb of space to 
 *   TPoolFile, but to merely provide much more than 2Gb.  So I have imposed a limit on the 
 *   number of files, MAX_OPEN_FILES, because it has to keep all the files opened.  Perhaps 
 *   I avoid keeping them all open at once, but again my purpose was just to provide more 
 *   than 2Gb of storage.   When 64bit file systems are the normal in a few years, this 
 *   won't even be an issue.
 *
 * - This class works by taking the address space and simply assigning the first 2 billion 
 *   addresses to the first file, then the next 2 billion addresses to the second file and
 *   so on.  However, there is a 512 byte header at the beginning of each file which is used
 *   to tie the files in a set together.
 *
 */

// TODO need to check return values of lseek

CMultiFile::CMultiFile() :
	opened(false),
	initialFilename(""),
	openFileCount(0)
{
	for(size_t t=0;t<MAX_OPEN_FILES;t++)
		openFiles[t]=-1;
}

CMultiFile::~CMultiFile()
{
	close(false);
}

void CMultiFile::open(const string _initialFilename,const bool canCreate)
{
	if(opened)
		throw runtime_error(string(__func__)+" -- already opened");

	initialFilename=_initialFilename;
	openFileCount=0;
	//position=0;

	try
	{
		if(canCreate)
			CPath(initialFilename).touch();

		// open first file
		RFileHeader header;
		openFile(initialFilename,header,true);
		matchSignature=header.matchSignature;

		// attempt to open all the other files
		for(uint64_t t=1;t<header.fileCount;t++)
			openFile(buildFilename(t),header);

		writeHeaderToFiles();

		opened=true;

		totalSize=calcTotalSize();
	}
	catch(...)
	{
		close();
		throw;
	}
}

void CMultiFile::close(bool removeFiles)
{
	for(size_t t=0;t<MAX_OPEN_FILES;t++)
	{
		if(openFiles[t]>0)
		{
			::close(openFiles[t]);
			openFiles[t]=-1;

			if(removeFiles)
				unlink(buildFilename(t).c_str());
		}
	}
	openFileCount=0;

	initialFilename="";
	//position=0;
	totalSize=0;
	opened=false;
}

void CMultiFile::rename(const string newInitialFilename)
{
	const size_t origOpenFileCount=openFileCount;
	const string origInitialFilename=initialFilename;
	close(false);

	for(size_t t=0;t<origOpenFileCount;t++)
		::rename(buildFilename(t,origInitialFilename).c_str(),buildFilename(t,newInitialFilename).c_str());

	open(newInitialFilename,false);
}

void CMultiFile::seek(const l_addr_t _position,RHandle &handle)
{
	if(_position>totalSize)
		throw runtime_error(string(__func__)+" -- attempting to seek beyond the end of the file size: "+istring(_position));
	handle.position=_position;
}

const CMultiFile::l_addr_t CMultiFile::tell(RHandle &handle) const
{
	return(handle.position);
}

void CMultiFile::read(void *buffer,const l_addr_t count,RHandle &handle)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- not opened");
	if((totalSize-handle.position)<count)
		throw runtime_error(string(__func__)+" -- attempting to read beyond the end of the file size; position: "+istring(handle.position)+" count: "+istring(count));

	size_t whichFile=handle.position/LOGICAL_MAX_FILE_SIZE;
	f_addr_t whereFile=(handle.position%LOGICAL_MAX_FILE_SIZE)+sizeof(RFileHeader);

	l_addr_t lengthToRead=count;
	while(lengthToRead>0)
	{

		const f_addr_t seekRet=lseek(openFiles[whichFile],whereFile,SEEK_SET);
		if(seekRet==((f_addr_t)-1))
		{
			int errNO=errno;
			throw runtime_error(string(__func__)+" -- error seeking in file: "+buildFilename(whichFile)+" -- strerror: "+strerror(errNO));
		}

		const size_t stripRead=min(lengthToRead,LOGICAL_MAX_FILE_SIZE-whereFile);
		const ssize_t lengthRead=::read(openFiles[whichFile],(uint8_t *)buffer+(count-lengthToRead),stripRead);
		if(lengthRead<0 || (size_t)lengthRead!=stripRead)
		{
			int errNO=errno;
			throw runtime_error(string(__func__)+" -- error reading from file: "+buildFilename(whichFile)+" -- lengthRead/stripRead: "+istring(lengthRead)+"/"+istring(stripRead)+" -- strerror: "+strerror(errNO));
		}

		lengthToRead-=stripRead;
		handle.position+=stripRead;

		whichFile++;			// read from the next file next go around
		whereFile=sizeof(RFileHeader);	// each additional file to read from should start at 0 now
	}
}

void CMultiFile::read(void *buffer,const l_addr_t count,const l_addr_t _position)
{
	RHandle handle;
	seek(_position,handle);
	read(buffer,count,handle);
}

void CMultiFile::write(const void *buffer,const l_addr_t count,RHandle &handle)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- not opened");
	if((getAvailableSize()-handle.position)<count)
		throw runtime_error(string(__func__)+" -- insufficient space to write "+istring(count)+" more bytes");

	// grow file(s) if necessary 
	if(totalSize<(handle.position+count))
		prvSetSize(handle.position+count,count);

	size_t whichFile=handle.position/LOGICAL_MAX_FILE_SIZE;
	f_addr_t whereFile=(handle.position%LOGICAL_MAX_FILE_SIZE)+sizeof(RFileHeader);

	l_addr_t lengthToWrite=count;
	while(lengthToWrite>0)
	{

		const f_addr_t seekRet=lseek(openFiles[whichFile],whereFile,SEEK_SET);
		if(seekRet==((f_addr_t)-1))
		{
			int errNO=errno;
			throw runtime_error(string(__func__)+" -- error seeking in file: "+buildFilename(whichFile)+" -- strerror: "+strerror(errNO));
		}

		const size_t stripWrite=min(lengthToWrite,LOGICAL_MAX_FILE_SIZE-whereFile);
		const ssize_t lengthWritten=::write(openFiles[whichFile],(uint8_t *)buffer+(count-lengthToWrite),stripWrite);
		if(lengthWritten<0 || (size_t)lengthWritten!=stripWrite)
		{
			int errNO=errno;
			throw runtime_error(string(__func__)+" -- error writing to file: "+buildFilename(whichFile)+" -- lengthWritten/stripWrite: "+istring(lengthWritten)+"/"+istring(stripWrite)+" -- strerror: "+strerror(errNO));
		}

		lengthToWrite-=lengthWritten;
		handle.position+=lengthWritten;

		whichFile++;			// write to the next file next go around
		whereFile=sizeof(RFileHeader);	// each additional file to write to should start at 0 now
	}
}

void CMultiFile::write(const void *buffer,const l_addr_t count,const l_addr_t _position)
{
	RHandle handle;
	seek(_position,handle);
	write(buffer,count,handle);
}

void CMultiFile::setSize(const l_addr_t newSize)
{
	prvSetSize(newSize,0);
}

void CMultiFile::prvSetSize(const l_addr_t newSize,const l_addr_t forWriteSize)
{
	if(newSize>totalSize && getAvailableSize()<newSize)
		throw runtime_error(string(__func__)+" -- insufficient space to grow data size to: "+istring(newSize));

	size_t neededFileCount=(newSize/LOGICAL_MAX_FILE_SIZE)+1;
	const f_addr_t currentLastFilesSize=(totalSize%LOGICAL_MAX_FILE_SIZE)+sizeof(RFileHeader);
	const f_addr_t lastFilesSize=(newSize%LOGICAL_MAX_FILE_SIZE)+sizeof(RFileHeader);

	if(neededFileCount>openFileCount)
	{ // create some new files
		if(neededFileCount>MAX_OPEN_FILES)
			throw runtime_error(string(__func__)+" -- would have to open too many files ("+istring(neededFileCount)+") to set size to "+istring(newSize));

		// set the last file now its max size
		setFileSize(openFiles[openFileCount-1],PHYSICAL_MAX_FILE_SIZE);

		// create new files and set all but the last to their max size
		for(size_t t=openFileCount;t<neededFileCount;t++)
		{
			const string filename=buildFilename(t);
			CPath(filename).touch();
			openFile(filename,*((RFileHeader *)NULL),false,false);
			if(t!=neededFileCount-1)
				setFileSize(openFiles[openFileCount-1],PHYSICAL_MAX_FILE_SIZE);
		}

		writeHeaderToFiles(); // write the new file count to all the files (could probably get by with just writing it to the first one???)
	}
	else
	{
		// close and remove some files
		for(size_t t=neededFileCount;t<openFileCount;t++)
		{
			::close(openFiles[t]);
			openFiles[t]=-1;
			unlink(buildFilename(t).c_str());
		}
		openFileCount=neededFileCount;

		writeHeaderToFiles(); // write the new file count to all the files (could probably get by with just writing it to the first one???)
	}

		

	// set size of last file
	if(newSize<totalSize || ((l_addr_t)lastFilesSize-forWriteSize)>(l_addr_t)currentLastFilesSize)
		setFileSize(openFiles[openFileCount-1],lastFilesSize-forWriteSize);
	totalSize=newSize;
}

void CMultiFile::sync() const
{ 
	/*
	for(size_t t=0;t<openFileCount;t++)
		fsync(openFiles[t]);
	*/
}

const CMultiFile::l_addr_t CMultiFile::getAvailableSize() const
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- not opened");

	return(MAX_OPEN_FILES*LOGICAL_MAX_FILE_SIZE);
}

const CMultiFile::l_addr_t CMultiFile::getActualSize() const
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- not opened");

	struct stat statBuf;
	fstat(openFiles[openFileCount-1],&statBuf);
	const l_addr_t sizeOfLastFile=statBuf.st_size;
	return(((openFileCount-1)*PHYSICAL_MAX_FILE_SIZE)+sizeOfLastFile);
}

const CMultiFile::l_addr_t CMultiFile::getSize() const
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- not opened");
	return(totalSize);
}

void CMultiFile::writeHeaderToFiles()
{
	RFileHeader header;

	header.signature=CMULTIFILE_SIGNATURE;
	header.matchSignature=matchSignature;
	header.fileCount=openFileCount;
	memset(header.padding,0,sizeof(header.padding));

	for(size_t t=0;t<openFileCount;t++)
	{
		lseek(openFiles[t],0,SEEK_SET);
		const ssize_t wroteLength=::write(openFiles[t],&header,sizeof(header));
		if(wroteLength<0 || (size_t)wroteLength!=sizeof(header))
		{
			int errNO=errno;
			throw runtime_error(string(__func__)+" -- error writing header to file -- wroteLength/sizeof(header): "+istring(wroteLength)+"/"+istring(sizeof(header))+" -- strerror: "+strerror(errNO));
		}
	}
}

void CMultiFile::openFile(const string &filename,RFileHeader &header,const bool openingFirstFile,const bool readHeader)
{
	int fileHandle=-1;
	try
	{
#ifdef WIN32
		fileHandle=sopen(filename.c_str(),O_RDWR|O_BINARY,SH_DENYRW);
#else
		// we need some way of not allowing other processes to open this file once it's open here
		fileHandle=::open(filename.c_str(),O_RDWR);
#endif
		if(fileHandle<0)
			throw runtime_error(string(__func__)+" -- expected file not found: "+filename+" -- "+strerror(errno));

		if(readHeader)
		{
			// read info which ties this file to other files
			if(::read(fileHandle,&header,sizeof(header))==sizeof(header) && header.signature==CMULTIFILE_SIGNATURE)
			{ // sizeof(header) bytes were read and signature matched

				if(openingFirstFile)
					matchSignature=header.matchSignature;
				else if(header.matchSignature!=matchSignature)
					throw runtime_error(string(__func__)+" -- matchSignature not match in header information of file: "+filename);

				if(header.fileCount>MAX_OPEN_FILES)
					throw runtime_error(string(__func__)+" -- fileCount is greater than MAX_OPEN_FILES in header information of file: "+filename);

				openFiles[openFileCount++]=fileHandle;
				fileHandle=-1;
			}
			else
			{
				if(openingFirstFile)
				{
					// make up new match signature 
					header.matchSignature=matchSignature=((rand()%256)*256*256*256)+((rand()%256)*256*256)+((rand()%256)*256)+(rand()%256);
					header.fileCount=openFileCount=1;

					openFiles[0]=fileHandle;
					fileHandle=-1;

				}
				else
					throw runtime_error(string(__func__)+" -- invalid header information of file: "+filename);
			}
		}
		else
		{
			openFiles[openFileCount++]=fileHandle;
			fileHandle=-1;
		}

	}
	catch(...)
	{
		if(fileHandle!=-1)
			::close(fileHandle);
		throw;
	}
}

const CMultiFile::l_addr_t CMultiFile::calcTotalSize() const
{
	return(getActualSize()-(openFileCount*sizeof(RFileHeader)));
}


void CMultiFile::setFileSize(const int fileHandle,const f_addr_t newFileSize)
{
/* Need to use flags from autoconf to set this
#if defined(WIN32)
	if(chsize(fileHandle,newFileSize)!=0)
	{
		int err=errno;
		printf("error changing block file size: %s\n",strerror(err));
		exit(1);
	}

#elif defined(__SOLARIS_GNU_CPP__) || defined(__SOLARIS_SUN_CPP__) || defined(__LINUX_GNU_CPP__)
*/
	if(ftruncate(fileHandle,newFileSize))
	{
		const int err=errno;
		if(err==EPERM)
		{ // "Operation not Permitted" (most likely this is an fat partition, or one that doesn't support sparse files)

			// In linux, the FAT file system does not support making the file bigger with ftruncate
			// I started a thread in the linux-fsdevel mailing list, but evenutually received thsi reply
			//  
			//     Exactly. We could extended file for you, but we decided that instead of
			//     writting zeroes to disk for next couple of hours during one ftruncate()
			//     call we leave decision on you - if you really want to extend file, use
			//     something else (loop in write and paint some progress bar for impatient 
			//     user). You can lookup discussion why Al Viro (if my memory serves 
			//     correctly) decided to not support extending files on filesystems
			//     which do not support holes - linux-fsdevel archive should contain it
			//     (I think that ~18 months ago, but I'm not sure at all).
			//                                         Best regards,
			//                                            Petr Vandrovec
			//                                            vandrove@vc.cvut.cz
			
			struct stat statBuf;
			fstat(fileHandle,&statBuf);
			const f_addr_t currentFileSize=statBuf.st_size;
			if(currentFileSize<=newFileSize)
			{ // write zero to the end of the file
				lseek(fileHandle,currentFileSize,SEEK_SET);

				const char buffer[1024]={0};
				const size_t nChunks=(newFileSize-currentFileSize)/1024;
				for(size_t t=0;t<nChunks;t++)
				{
					if(::write(fileHandle,buffer,1024)!=1024)
					{
						printf("error changing block file size: %s\n",strerror(errno));
						exit(1);
					}
				}
				const ssize_t remainder=(newFileSize-currentFileSize)%1024;
				if(remainder>0)
				{
					if(::write(fileHandle,buffer,remainder)!=remainder)
					{
						printf("error changing block file size: %s\n",strerror(errno));
						exit(1);
					}
				}

				return;
			}
		}

		// ??? may want to seriously consider making this an exception, but if I 
		// do I need know know all the conditions underwhich it could happen and 
		// undo whever might assume that the operation was going to succeed
		// 	- same for above in two places
		printf("error changing block file size: %s\n",strerror(err));
		exit(1);
	}
/*
#else
	#error no implementation for this platform defined
#endif
*/

}

const string CMultiFile::buildFilename(size_t which)
{
	return(buildFilename(which,initialFilename));
}


const string CMultiFile::buildFilename(size_t which,const string &initialFilename)
{
	if(which==0)
		return(initialFilename);
	else
		return(initialFilename+"."+istring(which));
}
