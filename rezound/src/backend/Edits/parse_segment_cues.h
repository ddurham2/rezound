#ifndef __parse_segment_cues_H__
#define __parse_segment_cues_H__

#include "../CSound.h"

#include <stddef.h>

#include <map>
#include <vector>
#include <string>
#include <utility>

using namespace std;

typedef vector<pair<string,pair<sample_pos_t,sample_pos_t> > > segments_t;

class FBuildFilename
{
public:
	virtual ~FBuildFilename() {}
	virtual const string operator()(const string str)=0;
};

static const segments_t parse_segment_cues(const CSound &sound,const sample_pos_t selectionStart,const sample_pos_t selectionLength,FBuildFilename &buildFilename)
{
	// built a list of a cues ordered by the time and keeping only the cues that start with '(' or ')'
	multimap<sample_pos_t,size_t> timeOrderedCueIndex;
	for(size_t t=0;t<sound.getCueCount();t++)
	{
		const string cueName=sound.getCueName(t);
		if(cueName.size()>0 && (cueName[0]=='(' || cueName[0]==')'))
		{
			const sample_pos_t cueTime=sound.getCueTime(t);
			if(cueTime>=selectionStart && (cueTime-selectionStart)<selectionLength)
				timeOrderedCueIndex.insert(make_pair(cueTime,t));
		}
	}

	// now create segments based on the '('..., ')' named cues

	/*
		State Machine:
				"("
				--- 
			       |   |
			 "("   V   |
		----> 0 -----> 1 -- 
		      ^         
		      |        |
		      |________|
			 ")"
	*/
	segments_t segments;
	sample_pos_t start=selectionStart;
	string filename="";
	int state=0;
	for(multimap<sample_pos_t,size_t>::iterator i=timeOrderedCueIndex.begin();i!=timeOrderedCueIndex.end();i++)
	{
		const string cueName=sound.getCueName(i->second);
		const sample_pos_t time=sound.getCueTime(i->second);
		const char c=cueName[0];
	
		switch(state)
		{
		case 0:
			if(c=='(')
			{
				filename=buildFilename(cueName.substr(1));
				start=time;
		
				state=1;
			}
			else
				throw runtime_error(string(__func__)+_(" -- syntax error in cue names -- expected to find a cue beginning with '(' at time: ")+sound.getTimePosition(time));

			break;

		case 1:
			if(c=='(')
			{
				segments.push_back(pair<string,pair<sample_pos_t,sample_pos_t> >(filename,pair<sample_pos_t,sample_pos_t>(start,time>0 ? time-1 : time)));

				filename=buildFilename(cueName.substr(1));
				start=time;

				state=1;
			}
			else if(c==')')
			{
				segments.push_back(pair<string,pair<sample_pos_t,sample_pos_t> >(filename,pair<sample_pos_t,sample_pos_t>(start,time)));
				// clear out for test after the loop
				filename="";
				start=selectionStart;

				state=0;
			}
			else
				throw runtime_error(string(__func__)+_(" -- syntax error in cue names -- expected to find a cue beginning with '(' or ')' at time: ")+sound.getTimePosition(time));

			break;
	
		default:
			throw runtime_error(string(__func__)+_(" -- internal error -- invalid state: ")+istring(state));
		}
	}


	// if a (... was encountered without a closing ) then we assume to save to the end of file (or we didn't create anything after a single '(')
	if(filename!="" && (start!=selectionStart || segments.empty()))
		segments.push_back(make_pair(filename,make_pair(start, selectionStart+selectionLength-1)));

	
	// remove segments with the same start and stop
	startOver:
	for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
	{
		if(i->second.first==i->second.second)
		{
			segments.erase(i);
			goto startOver; // cause I'm not sure if it's valid to delete from vector while pointing to it
		}
	}

	return segments;
}

#endif
