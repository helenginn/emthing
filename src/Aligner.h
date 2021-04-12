// em
// Copyright (C) 2019 Helen Ginn
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 
// Please email: vagabond @ hginn.co.uk for more details.

#ifndef __em__Aligner__
#define __em__Aligner__

#include <h3dsrc/Icosahedron.h>
#include <QObject>

class VagFFT; typedef boost::shared_ptr<VagFFT> VagFFTPtr;
class Any; typedef boost::shared_ptr<Any> AnyPtr;
class DistortMap; typedef boost::shared_ptr<DistortMap> DistortMapPtr;
class Symmetry;

class Aligner : public QObject, public Icosahedron
{
Q_OBJECT
public:
	Aligner(DistortMapPtr v1, DistortMapPtr v2);
	~Aligner();

	mat3x3 &result()
	{
		return _result;
	}

	DistortMapPtr reference()
	{
		return _ori0;
	}

	DistortMapPtr altered()
	{
		return _ori1;
	}
	
	size_t symScoreCount()
	{
		return _symScores.size();
	}
	
	double symScore(int i)
	{
		return _symScores[i];
	}
	
	double symRatio();

	void removeTrials(mat3x3 rot, double rad);
	void setSymmetry(std::string filename);
public slots:
	void align();

signals:
	void resultReady();
	void focusOnCentre();
private:
	void rotation();
	void translation();
	void bestSymmetry();
	void microAdjustments(double resol, int cycles, double threshold);
	void refineAdjustments(double resol, int cycles);
	void finishRefinement();
	void binToTargetDim(float dim);
	
	static double score(void *object)
	{
		return static_cast<Aligner *>(object)->adjustScore();

	}

	double adjustScore();

	DistortMapPtr subset(VagFFTPtr fft);
	mat3x3 makeRotation(vec3 axis, double deg);
	DistortMapPtr _v0, _v1;
	DistortMapPtr _ori0, _ori1;
	
	mat3x3 _result;
	vec3 _translation;

	bool forbidden(mat3x3 m);
	double _scale;
	std::vector<mat3x3> _forbidden;
	std::vector<double> _forbidAngles;
	vec3 _microTrans;
	vec3 _microAngles;

	Symmetry *_sym;
	std::vector<double> _symScores;
};

#endif
