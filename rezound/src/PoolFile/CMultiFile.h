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

#ifndef __CMultiFile_H__
#define __CMultiFile_H__

#include "../../config/common.h"

#include <stddef.h>
#include <stdint.h>

#include <string>

#include <sys/types.h> // for off_t and ssize_t

class CMultiFile
{
public:
	#define MAX_OPEN_FILES 64
	typedef uint64_t l_addr_t;	// logical address within the whole (note, this is NOT the l_addr_t given to TPoolFile, it's actually TPoolFile's p_addr_t)

	typedef off_t f_addr_t;		// physical address within a file

	CMultiFile();

	virtual ~CMultiFile();

	/* -- Opens the file(s)
	 * - initialFilename is the filename of the first file in the possible set of multiple files 
	 */
	void open(const string _initialFilename,const bool canCreate);
	bool isOpen() const { return opened; }
	void close(bool removeFiles=false);

	void rename(const string newInitialFilename);


	// The seek, tell, read and write methods require that one of these
	// be instantiated and a pointer be passed when the method is called
	class RHandle // ??? for lack of a better term
	{
	public:
		RHandle() { position=0; }
	private:
		friend class CMultiFile;
		CMultiFile::l_addr_t position;
	};

	void seek(const l_addr_t _position,RHandle &handle);
	const l_addr_t tell(RHandle &handle) const;

	void read(void *buffer,const l_addr_t count,RHandle &handle);
	void read(void *buffer,const l_addr_t count,const l_addr_t _position);

	void write(const void *buffer,const l_addr_t count,RHandle &handle);
	void write(const void *buffer,const l_addr_t count,const l_addr_t _position);

	void setSize(const l_addr_t newSize);

	void sync() const;

	const l_addr_t getAvailableSize() const;
	const l_addr_t getActualSize() const;
	const l_addr_t getSize() const;


private:
	bool opened;

	// ??? having this limits the max total size to only 
	// LOGICAL_MAX_FILE_SIZE * MAX_OPEN_FILES.  I could
	// conceive of a list which is all the open files and
	// we open and close the files as needed between 
	// operations.
	int openFiles[MAX_OPEN_FILES];

	string initialFilename;
	uint32_t matchSignature;

	l_addr_t totalSize;

	size_t openFileCount;

	// 512 byte header at the beginning of the file
	// I do not write(&RFileHeader) because padding can be different on each platform, the methods are used
	struct RFileHeader
	{
		uint32_t signature;		// signature for CMultiFile
		uint32_t matchSignature; 	// signature for this set of files
		uint64_t fileCount;		// number of files in this set
		
		// returns the number of bytes read or written
		ssize_t read(int fd);
		ssize_t write(int fd);
		void encodeEndianBeforeWrite();
	};

	void writeHeaderToFiles();

	void openFile(const string &filename,RFileHeader &header,const bool openingFirstFile=false,const bool readHeader=true);

	const l_addr_t calcTotalSize() const;

	void prvSetSize(const l_addr_t newSize,const l_addr_t forWriteSize);
	void setFileSize(const int fileHandle,const f_addr_t newFileSize);

	const string buildFilename(size_t which);
	static const string buildFilename(size_t which,const string &initialFilename);
};

#endif
