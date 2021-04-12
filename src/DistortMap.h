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

class DistortMap; typedef boost::shared_ptr<DistortMap> DistortMapPtr;
class AtomGroup; typedef boost::shared_ptr<AtomGroup> AtomGroupPtr;

class DistortMap : public SlipObject, public VagFFT
{
public:
	DistortMap(VagFFT &fft, int scratches = 0);
	DistortMap(int nx, int ny, int nz, int nele, int scratches);
	
	void maskWithAtoms(AtomGroupPtr atoms);

	void makeKeypoints(double distance);
	
	void setAuxiliary(std::string aux)
	{
		_aux = aux;
	}
	
	bool hasAuxiliary()
	{
		return (_aux.length() > 0);
	}

	void setFilename(std::string f)
	{
		_filename = f;
	}
	
	std::string filename()
	{
		return _filename;
	}
	
	double correlateKeypoints(DistortMapPtr other);

	void loadFromAuxiliary();
	
	virtual double cubic_interpolate(vec3 vox000, size_t im = false);
	
	struct Keypoint
	{
		vec3 start;
		vec3 shift;
		double correlation;
		double density;
		size_t vertex;
	};
	
	void setColour(double r, double g, double b)
	{
		_r = r; _g = g; _b = b;
	}

	void refineKeypoints(DistortMapPtr ref);
	void writeMRC(std::string filename, bool withDistortion = false);
	void dropData();
	
	void setFocus(bool f)
	{
		_focus = f;
	}
	
	void setReference(DistortMapPtr ref)
	{
		_reference = ref;
	}
	
	void setThreshold(float threshold)
	{
		_threshold = threshold;
	}

	int bestBin(float target);
	DistortMapPtr binned(int bin);

	double rotateRoundCentre(mat3x3 reindex, vec3 add, 
	                         DistortMapPtr other = DistortMapPtr(), 
	                         double scale = 1, bool write = false,
	                         bool useCentroid = false);
private:
	vec3 motion(vec3 real, std::vector<Keypoint *> &points, double *weights);
	void refineKeypoint(int i);
	double sscore();
	double aveSigma(bool ref = false);
	void prepareKeypoints();
	void get_limits(vec3 cur, double dist, vec3 *min, vec3 *max);
	void writeAuxiliary(std::string filename);

	static double score(void *object)
	{
		return static_cast<DistortMap *>(object)->sscore();
	}

	void closestPoints(vec3 real);
	void addIcosahedron();
	
	std::vector<Keypoint> _keypoints;
	std::vector<double> _aveDensities;
	std::vector<Keypoint *> _closest;
	std::vector<std::vector<Keypoint *> > _nnMap;
	Icosahedron *_ico;

	vec3 _current;
	std::string _aux;
	std::string _filename;
	bool _focus;
	double _distance;
	double _r, _g, _b;
	DistortMapPtr _reference;
	void initialise();
	float _threshold;
	
	std::vector<int> _pdbMask;
};

typedef boost::shared_ptr<DistortMap> DistortMapPtr;

#endif
