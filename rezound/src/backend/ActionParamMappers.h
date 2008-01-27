#ifndef __Mappers_H__
#define __Mappers_H__

#include "AActionParamMapper.h"

#include <math.h>
#include "../backend/unit_conv.h"

class CActionParamMapper_linear : public AActionParamMapper
{
public:
	CActionParamMapper_linear(double defaultValue,int defaultScalar,int minScalar=0,int maxScalar=0) :
		AActionParamMapper(defaultValue,defaultScalar,minScalar,maxScalar),
		doFloor(false)
	{
	}

	virtual ~CActionParamMapper_linear() {}

	void floorTheValue(bool _doFloor) { doFloor=_doFloor; }

	double interpretValue(const double x)
	{ // [0,1] -> [0,s] 
		if(doFloor)
			return floor(x*getScalar());
		else
			return x*getScalar();
	}

	double uninterpretValue(const double x)
	{ // [0,s] -> [0,1] 
		if(doFloor)
			return floor(x)/getScalar();
		else
			return x/getScalar();
	}

private:
	bool doFloor; // could make this a math.h function pointer so I could pass round, floor, ceil, etc???
};

// maps the unit range to [-scalar,scalar]
class CActionParamMapper_linear_bipolar : public AActionParamMapper
{
public:
	CActionParamMapper_linear_bipolar(double defaultValue,int defaultScalar,int maxScalar) :
		AActionParamMapper(defaultValue,defaultScalar,1,maxScalar),
		mul(1.0)
	{
	}

	void setMultiplier(double _mul) { mul=_mul; }


	virtual ~CActionParamMapper_linear_bipolar() {}

	double interpretValue(const double x)
	{ // [0,1] -> [-s,s]*mul
		return unitRange_to_otherRange_linear(x,-getScalar(),getScalar())*mul;
	}

	double uninterpretValue(const double x)
	{ // [-s,s]*mul -> [0,1] 
		return otherRange_to_unitRange_linear(x/mul,-getScalar(),getScalar());
	}

private:
	double mul;
};

// same as CActionParamMapper_linear_range below except the value will be multipled by the scalar
class CActionParamMapper_linear_range_scaled : public AActionParamMapper
{
public:
	CActionParamMapper_linear_range_scaled(double defaultValue,double _minValue,double _maxValue,int defaultScalar,int minScalar,int maxScalar) :
		AActionParamMapper(defaultValue,defaultScalar,minScalar,maxScalar),
		minValue(_minValue),
		maxValue(_maxValue),
		doFloor(false)
	{
	}

	virtual ~CActionParamMapper_linear_range_scaled() {}

	void floorTheValue(bool _doFloor) { doFloor=_doFloor; }

	double interpretValue(const double x)
	{ // [0,1] -> [minValue,maxValue]*scalar 
		if(doFloor)
			return floor(unitRange_to_otherRange_linear(x,minValue,maxValue)*getScalar());
		else
			return unitRange_to_otherRange_linear(x,minValue,maxValue)*getScalar();
	}

	double uninterpretValue(const double x)
	{ // [minValue,maxValue]*scalar -> [0,1] 
		if(doFloor)
			return otherRange_to_unitRange_linear(floor(x)/getScalar(),minValue,maxValue);
		else
			return otherRange_to_unitRange_linear(x/getScalar(),minValue,maxValue);
	}

private:
	const double minValue,maxValue;
	bool doFloor; // could make this a math.h function pointer so I could pass round, floor, ceil, etc???
};

// maps the unit range to the given min and max values
class CActionParamMapper_linear_range : public CActionParamMapper_linear_range_scaled
{
public:
	CActionParamMapper_linear_range(double defaultValue,double minValue,double maxValue) :
		CActionParamMapper_linear_range_scaled(defaultValue,minValue,maxValue,1,0,0)
	{
	}

	virtual ~CActionParamMapper_linear_range() {}
};

// maps the unit range to [min,scalar]
class CActionParamMapper_linear_min_to_scalar : public AActionParamMapper
{
public:
	CActionParamMapper_linear_min_to_scalar(double defaultValue,double _minValue,int defaultScalar,int minScalar,int maxScalar) :
		AActionParamMapper(defaultValue,defaultScalar,minScalar,maxScalar),
		minValue(_minValue),
		doFloor(false)
	{
	}

	virtual ~CActionParamMapper_linear_min_to_scalar() {}

	void floorTheValue(bool _doFloor) { doFloor=_doFloor; }

	double interpretValue(const double x)
	{ // [0,1] -> [minValue,scalar]
		if(doFloor)
			return floor(unitRange_to_otherRange_linear(x,minValue,getScalar()));
		else
			return unitRange_to_otherRange_linear(x,minValue,getScalar());
	}

	double uninterpretValue(const double x)
	{ // [minValue,scalar] -> [0,1] 
		if(doFloor)
			return otherRange_to_unitRange_linear(x,minValue,getScalar());
		else
			return otherRange_to_unitRange_linear(x,minValue,getScalar());
	}

private:
	const double minValue;
	bool doFloor;
};

// same as CActionParamMapper_linear_range except the path from minValue to maxValue follows the curve shape: x^2 where x:[0,1] 
class CActionParamMapper_parabola_range : public AActionParamMapper
{ // ??? if I create a "CActionParamMapper_parabola_range_scaled" (like I did with linear) then make this class derive from it
public:
	CActionParamMapper_parabola_range(double defaultValue,double _minValue,double _maxValue) :
		AActionParamMapper(defaultValue),
		minValue(_minValue),
		maxValue(_maxValue)
	{
	}

	virtual ~CActionParamMapper_parabola_range() {}

	double interpretValue(const double x)
	{ // [0,1] -> [minValue,maxValue] 
		return unitRange_to_otherRange_linear(unitRange_to_unitRange_squared(x),minValue,maxValue);
	}

	double uninterpretValue(const double x)
	{ // [minValue,maxValue] -> [0,1] 
		return unitRange_to_unitRange_unsquared(otherRange_to_unitRange_linear(x,minValue,maxValue));
	}

private:
	const double minValue,maxValue;
};

/* 'recipsym' means reciprocally symetric as the range of the function is reciprocally symetric around 1 */
class CActionParamMapper_recipsym : public AActionParamMapper
{
public:
	CActionParamMapper_recipsym(double defaultValue,int defaultScalar,int minScalar,int maxScalar) :
		AActionParamMapper(defaultValue,defaultScalar,minScalar,maxScalar)
	{
	}

	virtual ~CActionParamMapper_recipsym() {}

	double interpretValue(const double x)
	{ // [0,1] -> [1/s,s] 
		return unitRange_to_recipsymRange_exp(x,getScalar());
	}

	double uninterpretValue(const double x)
	{ // [1/s,s] -> [0,1] 
		return recipsymRange_to_unitRange_exp(x,getScalar());
	}
};

class CActionParamMapper_dBFS : public AActionParamMapper
{
public:
	CActionParamMapper_dBFS(double defaultValue) :
		AActionParamMapper(defaultValue)
	{
	}

	virtual ~CActionParamMapper_dBFS() {}

	double interpretValue(const double x)
	{ // [0,1] -> [-inf,0]
		return amp_to_dBFS(x,1.0);
	}

	double uninterpretValue(const double x)
	{ // [-inf,0] -> [0,1] 
		return dBFS_to_amp(x,1.0);
	}
};


#endif
