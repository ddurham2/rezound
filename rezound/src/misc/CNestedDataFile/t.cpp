#include <stdio.h>

#include "CNestedDataFile.h"

int main()
{
	try
	{
		CNestedDataFile s("test.dat");

	}
	catch(exception &e)
	{
		fprintf(stderr,"exception caught -- %s\n",e.what());
	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught\n");
	}



	return(0);
}


