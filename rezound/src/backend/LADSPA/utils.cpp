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
 *
 * Some of this code was derived from the search.c, default.c and load.c files
 * that come with the LADSPA SDK
 */

#include "utils.h"

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <set>
#include <string>
#include <CPath.h>

#include "../settings.h"




static void LADSPADirectoryPluginSearch (const char * pcDirectory, vector<const LADSPA_Descriptor *> &list);

const vector<const LADSPA_Descriptor *> findLADSPAPlugins() {
  vector<const LADSPA_Descriptor *> list;

  char * pcBuffer;
  const char * pcEnd;
  const char * pcLADSPAPath;
  const char * pcStart;

  pcLADSPAPath = gLADSPAPath.c_str();
  if (pcLADSPAPath=="")
    return list;

  set<string> alreadySearched;
  
  pcStart = pcLADSPAPath;
  while (*pcStart != '\0') {
    pcEnd = pcStart;
    while (*pcEnd != ':' && *pcEnd != '\0')
      pcEnd++;
    
    pcBuffer = (char *)malloc(1 + pcEnd - pcStart);
    if (pcEnd > pcStart)
      strncpy(pcBuffer, pcStart, pcEnd - pcStart);
    pcBuffer[pcEnd - pcStart] = '\0';

    // use this because it resolves symbolic links and relative paths to be equivalent
    string realPath=CPath(pcBuffer).realPath();

    free(pcBuffer);
	
    if(alreadySearched.find(realPath)==alreadySearched.end())
    {
        LADSPADirectoryPluginSearch(realPath.c_str(), list);
        alreadySearched.insert(realPath);
    }

    pcStart = pcEnd;
    if (*pcStart == ':')
      pcStart++;
  }

  return list;
}

void LADSPADirectoryPluginSearch (const char * pcDirectory, vector<const LADSPA_Descriptor *> &list) {
  char * pcFilename;
  DIR * psDirectory;
  LADSPA_Descriptor_Function fDescriptorFunction;
  long lDirLength;
  long iNeedSlash;
  struct dirent * psDirectoryEntry;
  void * pvPluginHandle;

  lDirLength = strlen(pcDirectory);
  if (!lDirLength)
    return;
  if (pcDirectory[lDirLength - 1] == '/')
    iNeedSlash = 0;
  else
    iNeedSlash = 1;

  psDirectory = opendir(pcDirectory);
  if (!psDirectory)
    return;

  while (1) {

    psDirectoryEntry = readdir(psDirectory);
    if (!psDirectoryEntry) {
      closedir(psDirectory);
      return;
    }

    pcFilename = (char *)malloc(lDirLength
			+ strlen(psDirectoryEntry->d_name)
			+ 1 + iNeedSlash);
    strcpy(pcFilename, pcDirectory);
    if (iNeedSlash)
      strcat(pcFilename, "/");
    strcat(pcFilename, psDirectoryEntry->d_name);
    
    pvPluginHandle = dlopen(pcFilename, RTLD_LAZY);
    if (pvPluginHandle) {
      /* This is a file and the file is a shared library! */

      dlerror();
      fDescriptorFunction = (LADSPA_Descriptor_Function)dlsym(pvPluginHandle, "ladspa_descriptor");
      if (dlerror() == NULL && fDescriptorFunction) {
	/* We've successfully found a ladspa_descriptor function. 
	   Now add all the plugins obtainable from this descriptor function */
	const LADSPA_Descriptor *desc;
	for(int i=0; (desc=fDescriptorFunction(i)) ; i++)
		list.push_back(desc);

/* originally called a call-back function
	fCallbackFunction(pcFilename,
			  pvPluginHandle,
			  fDescriptorFunction);
*/

	free(pcFilename);
      }
      else {
	/* It was a library, but not a LADSPA one. Unload it. */
	dlclose(pcFilename);
	free(pcFilename);
      }
    }
  }
}




#include <math.h>

int getLADSPADefault(const LADSPA_PortRangeHint * psPortRangeHint, const unsigned long lSampleRate, LADSPA_Data * pfResult) {
  int iHintDescriptor;

  iHintDescriptor = psPortRangeHint->HintDescriptor & LADSPA_HINT_DEFAULT_MASK;

  switch (iHintDescriptor & LADSPA_HINT_DEFAULT_MASK) {
  case LADSPA_HINT_DEFAULT_NONE:
    return -1;
  case LADSPA_HINT_DEFAULT_MINIMUM:
    *pfResult = psPortRangeHint->LowerBound;
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_LOW:
    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
      *pfResult = exp(log(psPortRangeHint->LowerBound) * 0.75
		      + log(psPortRangeHint->UpperBound) * 0.25);
    }
    else {
      *pfResult = (psPortRangeHint->LowerBound * 0.75
		   + psPortRangeHint->UpperBound * 0.25);
    }
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_MIDDLE:
    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
      *pfResult = sqrt(psPortRangeHint->LowerBound
		       * psPortRangeHint->UpperBound);
    }
    else {
      *pfResult = 0.5 * (psPortRangeHint->LowerBound
			 + psPortRangeHint->UpperBound);
    }
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_HIGH:
    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
      *pfResult = exp(log(psPortRangeHint->LowerBound) * 0.25
		      + log(psPortRangeHint->UpperBound) * 0.75);
    }
    else {
      *pfResult = (psPortRangeHint->LowerBound * 0.25
		   + psPortRangeHint->UpperBound * 0.75);
    }
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_MAXIMUM:
    *pfResult = psPortRangeHint->UpperBound;
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_0:
    *pfResult = 0;
    return 0;
  case LADSPA_HINT_DEFAULT_1:
    *pfResult = 1;
    return 0;
  case LADSPA_HINT_DEFAULT_100:
    *pfResult = 100;
    return 0;
  case LADSPA_HINT_DEFAULT_440:
    *pfResult = 440;
    return 0;
  }

  /* We don't recognise this default flag. It's probably from a more
     recent version of LADSPA. */
  return -1;
}


// --------------------------------------------------------

const CPluginMapping getLADSPADefaultMapping(const LADSPA_Descriptor *plugin,const CSound *sound)
{
	unsigned inputPortCount=0;
	unsigned outputPortCount=0;
	//printf("plugin: %s\n",plugin->Name);
	for(size_t t=0;t<plugin->PortCount;t++)
	{
		const LADSPA_PortDescriptor pd=plugin->PortDescriptors[t];
		if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
		{
			//printf(" input audio port: %s\n",plugin->PortNames[t]);
			inputPortCount++;
		}
		else if(LADSPA_IS_PORT_OUTPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
		{
			//printf("output audio port: %s\n",plugin->PortNames[t]);
			outputPortCount++;
		}
	}

	return CPluginMapping::getDefaultMapping(plugin->Name,inputPortCount,outputPortCount,sound);
}

