#ifndef __CMacroPlayer_H__
#define __CMacroPlayer_H__

#include <string>
#include <vector>
using namespace std;

class ASoundFileManager;
class CNestedDataFile;

class CMacroPlayer
{
public:
	static const vector<string> getMacroNames(const CNestedDataFile *macroStore);

	CMacroPlayer(const CNestedDataFile *macroStore,const string macroName);
	virtual ~CMacroPlayer();

	bool doMacro(ASoundFileManager *soundFileManager,unsigned &count);

private:
	const CNestedDataFile *macroStore;
	const string macroName;
};

#endif
