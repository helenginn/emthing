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

#ifndef __em__DistortMap__
#define __em__DistortMap__

#include <libsrc/FFT.h>
#include <h3dsrc/Icosahedron.h>

class DistortMap : public SlipObject, public VagFFT
{
public:
	DistortMap(VagFFT &fft, int scratches = 0);

	void makeKeypoints(double distance);
	
	virtual double cubic_interpolate(vec3 vox000, size_t im = false);
	
	struct Keypoint
	{
		vec3 start;
		vec3 shift;
		size_t vertex;
	};
	
	void setColour(double r, double g, double b)
	{
		_r = r; _g = g; _b = b;
	}

	void refineKeypoints(VagFFTPtr ref);
	void writeMRC(std::string filename);
private:
	void refineKeypoint(int i);
	double sscore();
	double aveSigma();
	void get_limits(vec3 cur, double dist, vec3 *min, vec3 *max);

	static double score(void *object)
	{
		return static_cast<DistortMap *>(object)->sscore();
	}

	void closestPoints(vec3 real);
	void addIcosahedron();
	
	std::vector<Keypoint> _keypoints;
	std::vector<Keypoint *> _closest;
	std::vector<std::vector<Keypoint *> > _nnMap;
	Icosahedron *_ico;

	vec3 _current;
	double _distance;
	double _r, _g, _b;
	VagFFTPtr _reference;
};

typedef boost::shared_ptr<DistortMap> DistortMapPtr;

#endif
