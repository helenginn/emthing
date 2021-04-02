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

#ifndef __em__Trace__
#define __em__Trace__

#include <h3dsrc/SlipObject.h>

class VagFFT; typedef boost::shared_ptr<VagFFT> VagFFTPtr;

class Trace : public QObject, public SlipObject
{
Q_OBJECT
public:
	Trace();
	
	void setFFT(VagFFTPtr fft)
	{
		_fft = fft;
	}
	
	void setStart(vec3 start)
	{
		_start = start;
	}
	
	void setMinimum(double min)
	{
		_min = min;
	}

public slots:
	void trace();
	
signals:
	void resultReady();

private:
	void makeTrials();
	bool findNextBest(vec3 *next);
	void go_back(size_t times = 1);
	double score(vec3 dir, vec3 last, vec3 trial, bool first);
	double _min;

	std::vector<vec3> _trials;
	std::vector<vec3> _bad;
	
	std::vector<Helen3D::Vertex> _longestV;
	std::vector<GLuint> _longestI;
	VagFFTPtr _fft;
	vec3 _start;

};

#endif
