/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include "ClibaudiofileSoundTranslator.h"

#ifdef HAVE_LIBAUDIOFILE

#include <unistd.h> // for unlink

#include <stdexcept>
#include <vector>
#include <utility>

#include <istring>

#include <CPath.h>

#include "CSound.h"
#include "AStatusComm.h"
#include "AFrontendHooks.h"

#if (LIBAUDIOFILE_MAJOR_VERSION*10000+LIBAUDIOFILE_MINOR_VERSION*100+LIBAUDIOFILE_MICRO_VERSION) >= /*000204*/204
	#define HANDLE_CUES_AND_MISC
#else
	#warning LIBAUDIOFILE VERSION IS LESS THAN 0.2.4 AND SAVING AND LOADING OF CUES AND USER NOTES WILL BE DISABLED
#endif

static int getUserNotesMiscType(int type)
{
	if(type==AF_FILE_WAVE)
		return AF_MISC_ICMT;
	else if(type==AF_FILE_AIFFC || type==AF_FILE_AIFF)
		return AF_MISC_ANNO;

	return 0;
}


ClibaudiofileSoundTranslator::ClibaudiofileSoundTranslator()
{
}

ClibaudiofileSoundTranslator::~ClibaudiofileSoundTranslator()
{
}

static string errorMessage;
static void errorFunction(long code,const char *msg)
{
	errorMessage=msg;
}

	// ??? could just return a CSound object an have used the one constructor that takes the meta info
	// ??? but, then how would I be able to have createWorkingPoolFileIfExists
bool ClibaudiofileSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	// passing AF_NULL_FILESETUP makes it detect the parameters from the file itself
	return loadSoundGivenSetup(filename,sound,AF_NULL_FILESETUP);
}

bool ClibaudiofileSoundTranslator::loadSoundGivenSetup(const string filename,CSound *sound,AFfilesetup initialSetup) const
{
	bool ret=true;

	errorMessage="";
	afSetErrorHandler(errorFunction);

	AFfilehandle h=afOpenFile(filename.c_str(),"r",initialSetup);
	if(h==AF_NULL_FILEHANDLE)
		throw runtime_error(string(__func__)+_(" -- error opening")+" '"+filename+"' -- "+errorMessage);

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{

#if defined(SAMPLE_TYPE_S16)
		if(afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(sample_t)*8)!=0)
#elif defined(SAMPLE_TYPE_FLOAT)
		if(afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_FLOAT,sizeof(sample_t)*8)!=0)
#else
		#error unhandled SAMPLE_TYPE_xxx define
#endif
			throw runtime_error(string(__func__)+" -- error setting virtual format -- "+errorMessage);
			
#ifdef WORDS_BIGENDIAN
		if(afSetVirtualByteOrder(h,AF_DEFAULT_TRACK,AF_BYTEORDER_BIGENDIAN))
#else
		if(afSetVirtualByteOrder(h,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN))
#endif
			throw runtime_error(string(__func__)+" -- error setting virtual byte order -- "+errorMessage);

		unsigned channelCount=afGetChannels(h,AF_DEFAULT_TRACK);
		if(channelCount<=0 || channelCount>MAX_CHANNELS) // ??? could just ignore the extra channels
			throw runtime_error(string(__func__)+" -- invalid number of channels in audio file: "+istring(channelCount)+" -- you could simply increase MAX_CHANNELS in CSound.h");
		unsigned sampleRate=(unsigned)afGetRate(h,AF_DEFAULT_TRACK);

		if(sampleRate<4000 || sampleRate>196000)
			throw runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate)+" probably indicates a corrupt file and SGI's libaudiofile has been known to miss these");

		
		sound->createWorkingPoolFile(filename,sampleRate,channelCount,10*sampleRate); // start out with 10 seconds of space

		// remember information for how to save the file if libaudiofile is also used to save it
		{
			int sampleFormat,sampleWidth,compressionType;

			afGetSampleFormat(h,AF_DEFAULT_TRACK,&sampleFormat,&sampleWidth);
			compressionType=afGetCompression(h,AF_DEFAULT_TRACK);

			sound->getGeneralDataAccesser<int>("AF_SAMPFMT_xxx").write(&sampleFormat,1);
			sound->getGeneralDataAccesser<int>("AF_sample_width").write(&sampleWidth,1);
			sound->getGeneralDataAccesser<int>("AF_COMPRESSION_xxx").write(&compressionType,1);
		}

#ifdef HANDLE_CUES_AND_MISC

		// load the cues
		const size_t cueCount=afGetMarkIDs(h,AF_DEFAULT_TRACK,NULL);
		if(cueCount<4096) // check for a sane amount
		{
			sound->clearCues();

			std::vector<int> markIDs(cueCount);
			afGetMarkIDs(h,AF_DEFAULT_TRACK,markIDs.data());
			for(size_t t=0;t<cueCount;t++)
			{
				string name=afGetMarkName(h,AF_DEFAULT_TRACK,markIDs[t]);
				if(name=="")
					name=sound->getUnusedCueName("cue"); // give it a unique name

				const sample_pos_t time=afGetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[t]);

				// to determine if the cue is anchored or not, name will be:
				// "asdf|+" if anchored, or "asdf|-" or "asdf" if not anchored
				const size_t delimPos=name.rfind("|");
				bool isAnchored=false;
				if(delimPos!=string::npos)
				{
					isAnchored=name.substr(delimPos+1)=="+";
					name.erase(delimPos);
				}

				try
				{
					sound->addCue(name,time,isAnchored);
				}
				catch(exception &e)
				{ // don't make an error adding a cue cause the whole file not to load
					Warning("file: '"+filename+"' -- "+e.what());
				}
			}
		}
		else
			Warning("Insane amount of markers so they were ignored: "+istring(cueCount)+" -- "+filename);
		

		// load the user notes
		if(afGetMiscIDs(h,NULL)>0 /*this is not implemented in libaudiofile yet && afQueryLong(AF_QUERYTYPE_MISC,AF_QUERY_SUPPORTED,fileFormatType,0,0)*/)
		{
			// ??? actually there are probably several AF_MISC_XXX that I would want to include in the user notes... just separate each one found with a newline
			// find the misc ID of the user notes
			int _v;
			const int userNotesMiscType=getUserNotesMiscType(afGetFileFormat(h,&_v));
			std::vector<int> miscIDs(afGetMiscIDs(h,NULL));
			const int miscIDCount=afGetMiscIDs(h,miscIDs.data());
			int userNotesMiscID=-1;
			for(int t=0;t<miscIDCount;t++)
			{
				if(afGetMiscType(h,miscIDs[t])==userNotesMiscType)
				{
					userNotesMiscID=miscIDs[t];
					break;
				}
			}

			if(userNotesMiscID!=-1) // was found
			{
				const int userNotesLength=afGetMiscSize(h,userNotesMiscID);
				std::vector<int8_t> buf(userNotesLength);
				afReadMisc(h,userNotesMiscID,buf.data(),userNotesLength);
				sound->setUserNotes(string((char *)(buf.data()),userNotesLength));
			}
		}
		
#endif // HANDLE_CUES_AND_MISC

		// load the audio data

		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));

		std::vector<sample_t> buffer((size_t)(afGetVirtualFrameSize(h,AF_DEFAULT_TRACK,1)*4096/sizeof(sample_t)));
		sample_pos_t pos=0;
		CStatusBar statusBar(_("Loading Sound"),0,afGetFrameCount(h,AF_DEFAULT_TRACK),true);
		for(;;)
		{
			const int read=afReadFrames(h,AF_DEFAULT_TRACK,buffer.data(),4096);
			if(read>0)
			{
				// increase length of file by 10 seconds if necessary
				if((pos+read)>accessers[0]->getSize())
					sound->addSpace(sound->getLength(),10*sampleRate);

				for(unsigned c=0;c<channelCount;c++)
				{
					for(int i=0;i<read;i++)
						(*(accessers[c]))[pos+i]=buffer[i*channelCount+c];
				}
				pos+=read;
			}
			else
				break; // done reading

			if(statusBar.update(pos))
			{ // cancelled
				ret=false;
				goto cancelled;
			}
		}

		cancelled:

		// remove unnecessary space
		if(sound->getLength()>pos)
			sound->removeSpace(pos,sound->getLength()-pos);

		afCloseFile(h);

		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}
	}
	catch(...)
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];
		afCloseFile(h);
		throw;
	}
	return ret;
}

static int sampleTypeToIndex(int sampleType)
{
	switch(sampleType)
	{
		case AF_SAMPFMT_TWOSCOMP: return 0;
		case AF_SAMPFMT_UNSIGNED: return 1;
		case AF_SAMPFMT_FLOAT: return 2;
		case AF_SAMPFMT_DOUBLE: return 3;
		default: throw runtime_error(string(__func__)+" -- internal error -- unhandled sampleType: "+istring(sampleType));
	};
}

static int sampleWidthToIndex(int sampleWidth)
{
	switch(sampleWidth)
	{
		case 8: return 0;
		case 16: return 1;
		case 24: return 2;
		case 32: return 3;
		default: {printf("weird sampleWidth: %d -- returning 16bit instead\n",sampleWidth); return 1;}
	};
}

static int compressionTypeToIndex(int compressionType,vector<pair<string,int> > supportedCompressionTypes)
{
	for(size_t t=0;t<supportedCompressionTypes.size();t++)
	{
		if(supportedCompressionTypes[t].second==compressionType)
			return t;
	}

	return 0; // none
}

bool ClibaudiofileSoundTranslator::onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs) const
{
	int fileType;

	const string extension=istring(CPath(filename).extension()).lower();
	if(extension=="wav")
		fileType=AF_FILE_WAVE;
	else if(extension=="aiff")
		fileType=AF_FILE_AIFF;
	else if(extension=="snd" || extension=="au")
		fileType=AF_FILE_NEXTSND;
	else if(extension=="sf")
		fileType=AF_FILE_BICSF;
	else
		throw runtime_error(string(__func__)+" -- unhandled extension for filename: "+filename);

	AFfilesetup setup=afNewFileSetup();
	try
	{
		// get user preferences for saving the file
		static bool parametersGotten=false;
		static AFrontendHooks::libaudiofileSaveParameters parameters;
		useLastUserPrefs&=parametersGotten;
		if(!useLastUserPrefs)
		{
			// get name of format to show
			const char *desc=(const char *)afQueryPointer(AF_QUERYTYPE_FILEFMT,AF_QUERY_NAME,fileType,0,0);

			// setup for supported compression types
			{
				int nCompressionTypes=afQueryLong(AF_QUERYTYPE_FILEFMT,AF_QUERY_COMPRESSION_TYPES,AF_QUERY_VALUE_COUNT,fileType,0);
				const int *compressionTypes=(const int *)afQueryPointer(AF_QUERYTYPE_FILEFMT,AF_QUERY_COMPRESSION_TYPES,AF_QUERY_VALUES,fileType,0); // I may need to free this??? but I haven't seen in the docs that it needs to be done, ask him
				vector<pair<string,int> > supportedCompressionTypes;
				supportedCompressionTypes.push_back(make_pair(string("None"),(int)AF_COMPRESSION_NONE));
				for(int t=0;t<nCompressionTypes;t++)
				{
					supportedCompressionTypes.push_back(make_pair(
						(const char *)afQueryPointer(AF_QUERYTYPE_COMPRESSION,AF_QUERY_NAME,compressionTypes[t],0,0),
						compressionTypes[t]
					));
				}
			
				parameters.supportedCompressionTypes=supportedCompressionTypes;
			}

			parameters.defaultSampleFormatIndex=
				sound->containsGeneralDataPool("AF_SAMPFMT_xxx") 
				? 
					sampleTypeToIndex(sound->getGeneralDataAccesser<int>("AF_SAMPFMT_xxx")[0])
				: 
					sampleTypeToIndex(afQueryLong(AF_QUERYTYPE_FILEFMT,AF_QUERY_SAMPLE_FORMATS,AF_QUERY_DEFAULT,fileType,0))
				;


			parameters.defaultSampleWidthIndex=
				sound->containsGeneralDataPool("AF_sample_width") 
				? 
					sampleWidthToIndex(sound->getGeneralDataAccesser<int>("AF_sample_width")[0])
				: 
					sampleWidthToIndex(afQueryLong(AF_QUERYTYPE_FILEFMT,AF_QUERY_SAMPLE_SIZES,AF_QUERY_DEFAULT,fileType,0))
				;

			parameters.defaultCompressionTypeIndex=
				sound->containsGeneralDataPool("AF_COMPRESSION_xxx") 
				? 
					compressionTypeToIndex(sound->getGeneralDataAccesser<int>("AF_COMPRESSION_xxx")[0],parameters.supportedCompressionTypes)
				: 
					compressionTypeToIndex(AF_COMPRESSION_NONE,parameters.supportedCompressionTypes)
				;

			if(!gFrontendHooks->promptForlibaudiofileSaveParameters(parameters,desc))
				return false;
			parametersGotten=true;
		}

		
		// ??? all the following parameters need to be passed in somehow as the export format
		// 	??? can easily do it with AFrontendHooks
		afInitFileFormat(setup,fileType); 
		if(fileType==AF_FILE_WAVE)
			afInitByteOrder(setup,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN);
		else if(fileType==AF_FILE_AIFF)
			afInitByteOrder(setup,AF_DEFAULT_TRACK,AF_BYTEORDER_BIGENDIAN);
		else if(fileType==AF_FILE_NEXTSND)
			afInitByteOrder(setup,AF_DEFAULT_TRACK,AF_BYTEORDER_BIGENDIAN);
		else
		{	// let the endian be native
#ifdef WORDS_BIGENDIAN
			afInitByteOrder(setup,AF_DEFAULT_TRACK,AF_BYTEORDER_BIGENDIAN);
#else
			afInitByteOrder(setup,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN);
#endif
		}
		afInitChannels(setup,AF_DEFAULT_TRACK,sound->getChannelCount());
		afInitSampleFormat(setup,AF_DEFAULT_TRACK,parameters.sampleFormat,parameters.sampleWidth);
		afInitRate(setup,AF_DEFAULT_TRACK,sound->getSampleRate()); // ??? would put on the dialog except the library doesn't do sample rate conversion itself currently
		afInitCompression(setup,AF_DEFAULT_TRACK,parameters.compressionType);
		afInitFrameCount(setup,AF_DEFAULT_TRACK,saveLength);

		const bool ret=saveSoundGivenSetup(filename,sound,saveStart,saveLength,setup,fileType,useLastUserPrefs,parameters.saveCues,parameters.saveUserNotes);

		afFreeFileSetup(setup);

		return ret;
	}
	catch(...)
	{
		afFreeFileSetup(setup);
		throw;
	}
}


bool ClibaudiofileSoundTranslator::saveSoundGivenSetup(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength,AFfilesetup initialSetup,int fileFormatType,bool useLastUserPrefs,bool saveCues,bool saveUserNotes) const
{
	bool ret=true;

	errorMessage="";
	afSetErrorHandler(errorFunction);

	const unsigned channelCount=sound->getChannelCount();
	const unsigned sampleRate=sound->getSampleRate();

#ifdef HANDLE_CUES_AND_MISC

	// setup for saving the cues (except for positions)
	std::vector<int> markIDs(sound->getCueCount());
	if(saveCues)
	{
		size_t cueCount=0;
		for(size_t t=0;t<sound->getCueCount();t++)
		{
			if(sound->getCueTime(t)>=saveStart && sound->getCueTime(t)<(saveStart+saveLength))
				markIDs[cueCount]=cueCount++;
		}
		if(cueCount>0)
		{
			if(!afQueryLong(AF_QUERYTYPE_MARK,AF_QUERY_SUPPORTED,fileFormatType,0,0))
			{
				if(!useLastUserPrefs) /* don't warn user if they've probably already been warned */
					Warning(_("This format does not support saving cues"));
			}
			else
			{

				afInitMarkIDs(initialSetup,AF_DEFAULT_TRACK,markIDs.data(),(int)cueCount);
				size_t temp=0;
				for(size_t t=0;t<sound->getCueCount();t++)
				{
					if(sound->getCueTime(t)>=saveStart && sound->getCueTime(t)<(saveStart+saveLength))
					{
							// to indicate if the cue is anchored we append to the name:
							// "|+" or "|-" whether it is or isn't
							const string name=sound->getCueName(t)+"|"+(sound->isCueAnchored(t) ? "+" : "-");
							afInitMarkName(initialSetup,AF_DEFAULT_TRACK,markIDs[temp++],name.c_str());

							// can't save position yet because that function requires a file handle, but
							// we can't move this code after the afOpenFile, or it won't save cues at all
							//afSetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[temp++],sound->getCueTime(t));
					}
				}
			}
		}
	}


	// prepare to save the user notes
	const string userNotes=sound->getUserNotes();
	int userNotesMiscID=1;
	const int userNotesMiscType=getUserNotesMiscType(fileFormatType);
	if(saveUserNotes && userNotes.length()>0)
	{
		/* this is not implemented in libaudiofile yet
		 *  ??? if this is changed in cvs before the verison is bumped to 0.2.4, then I can just add this in because it would be disabled if the version wasn't >0.2.4
		if(!afQueryLong(AF_QUERYTYPE_MISC,AF_QUERY_SUPPORTED,fileFormatType,0,0))
			Warning("This format does not support saving user notes");
		else
		*/
		if(fileFormatType==AF_FILE_WAVE || fileFormatType==AF_FILE_AIFF)
		{
			afInitMiscIDs(initialSetup,&userNotesMiscID,1);
			afInitMiscType(initialSetup,userNotesMiscID,userNotesMiscType);
			afInitMiscSize(initialSetup,userNotesMiscID,userNotes.length());
		}
	}

#endif // HANDLE_CUES_AND_MISC


	unlink(filename.c_str());
	AFfilehandle h=afOpenFile(filename.c_str(),"w",initialSetup);
	if(h==AF_NULL_FILEHANDLE)
		throw runtime_error(string(__func__)+" -- error opening '"+filename+"' for writing -- "+errorMessage);
	try
	{
		// ??? this if set may not completly handle all possibilities
#if defined(SAMPLE_TYPE_S16)
		if(afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(sample_t)*8)!=0)
#elif defined(SAMPLE_TYPE_FLOAT)
		if(afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_FLOAT,sizeof(sample_t)*8)!=0)
#else
		#error unhandled SAMPLE_TYPE_xxx define
#endif
			throw runtime_error(string(__func__)+" -- error setting virtual format -- "+errorMessage);

#ifdef WORDS_BIGENDIAN
		if(afSetVirtualByteOrder(h,AF_DEFAULT_TRACK,AF_BYTEORDER_BIGENDIAN))
#else
		if(afSetVirtualByteOrder(h,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN))
#endif
			throw runtime_error(string(__func__)+" -- error setting virtual byte order -- "+errorMessage);
		afSetVirtualChannels(h,AF_DEFAULT_TRACK,channelCount);

		CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
		try
		{
			for(unsigned t=0;t<channelCount;t++)
				accessers[t]=new CRezPoolAccesser(sound->getAudio(t));
			
			// save the audio data
			std::vector<sample_t> buffer((size_t)(channelCount*4096));
			sample_pos_t pos=0;
			AFframecount count=saveLength/4096;
			CStatusBar statusBar(_("Saving Sound"),0,saveLength,true);
			for(AFframecount t=0;t<=count;t++)
			{
				const int chunkSize= (t==count) ? (saveLength%4096) : 4096;
				if(chunkSize!=0)
				{
					for(unsigned c=0;c<channelCount;c++)
					{
						for(int i=0;i<chunkSize;i++)
							buffer[i*channelCount+c]=(*(accessers[c]))[pos+i+saveStart];
					}
					if(afWriteFrames(h,AF_DEFAULT_TRACK,buffer.data(),chunkSize)!=chunkSize)
						throw runtime_error(string(__func__)+" -- error writing audio data -- "+errorMessage);
					pos+=chunkSize;
				}

				if(statusBar.update(pos))
				{ // cancelled
					ret=false;
					goto cancelled;
				}
			}

#ifdef HANDLE_CUES_AND_MISC
			
			// write the cue's positions
			if(saveCues && afQueryLong(AF_QUERYTYPE_MARK,AF_QUERY_SUPPORTED,fileFormatType,0,0))
			{
				size_t temp=0;
				for(size_t t=0;t<sound->getCueCount();t++)
				{
					if(sound->getCueTime(t)>=saveStart && sound->getCueTime(t)<(saveStart+saveLength))
						afSetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[temp++],sound->getCueTime(t)-saveStart);
				}
			}

			// write the user notes
			if(saveUserNotes && userNotes.length()>0)
			{
				/* this is not implemented in libaudiofile yet
				if(!afQueryLong(AF_QUERYTYPE_MISC,AF_QUERY_SUPPORTED,fileFormatType,0,0))
					Warning("This format does not support saving user notes");
				else
				*/
				if(fileFormatType==AF_FILE_WAVE || fileFormatType==AF_FILE_AIFF)
				{
					afWriteMisc(h,userNotesMiscID,(void *)userNotes.c_str(),userNotes.length());
				}
			}

#endif // HANDLE_CUES_AND_MISC

			cancelled:

			afCloseFile(h);

			for(unsigned t=0;t<MAX_CHANNELS;t++)
			{
				delete accessers[t];
				accessers[t]=NULL;
			}
		}
		catch(...)
		{
			// attempt to close the file ??? and free the setup???

			for(unsigned t=0;t<MAX_CHANNELS;t++)
				delete accessers[t];

			throw;
		}

		if(!ret)
			unlink(filename.c_str()); // remove the cancelled file
	}
	catch(...)
	{
		afCloseFile(h);
		throw;
	}

	return ret;
}


bool ClibaudiofileSoundTranslator::handlesExtension(const string extension,const string filename) const
{
	return extension=="wav" || extension=="aiff" || extension=="au" || extension=="snd" || extension=="sf";
}

bool ClibaudiofileSoundTranslator::supportsFormat(const string filename) const
{
	int fd=open(filename.c_str(),O_RDONLY);
	int _e=errno;
	if(fd==-1)
		throw runtime_error(string(__func__)+" -- error opening file '"+filename+"' -- "+strerror(_e));

	int implemented=0;
	int id=afIdentifyNamedFD(fd,filename.c_str(),&implemented);
	close(fd);

	return implemented;
}

const vector<string> ClibaudiofileSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("Wave/RIFF");
	names.push_back("AIFF");
	names.push_back("NeXT/Sun");
	names.push_back("Berkeley/IRCAM/CARL");

	return names;
}

const vector<vector<string> > ClibaudiofileSoundTranslator::getFormatFileMasks() const
{
	vector<vector<string> > list;
	vector<string> fileMasks;

	fileMasks.clear();
	fileMasks.push_back("*.wav");
	list.push_back(fileMasks);

	fileMasks.clear();
	fileMasks.push_back("*.aiff");
	list.push_back(fileMasks);

	fileMasks.clear();
	fileMasks.push_back("*.snd");
	fileMasks.push_back("*.au");
	list.push_back(fileMasks);

	fileMasks.clear();
	fileMasks.push_back("*.sf");
	list.push_back(fileMasks);

	return list;
}

#endif // HAVE_LIBAUDIOFILE
