#ifndef __TTempVarSetter_H__
#define __TTempVarSetter_H__

/* this class temporarily sets a variable to a value while the object exists */
template <typename T> class TTempVarSetter 
{
	T &variable;
	const T backupValue;
public:
	TTempVarSetter(T &_variable,const T &value) : 
		variable(_variable),
		backupValue(variable) 
	{ 
		variable=value;
	}

	~TTempVarSetter()
	{
		variable=backupValue;
	}
};

#endif
