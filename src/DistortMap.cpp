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

#include "DistortMap.h"
#include <h3dsrc/shaders/vStructure.h>
#include <h3dsrc/shaders/fStructure.h>
#include <hcsrc/RefinementNelderMead.h>
#include <hcsrc/maths.h>
#include <hcsrc/Any.h>
#include <iostream>
#include <fstream>

DistortMap::DistortMap(VagFFT &fft, int scratch) : VagFFT(fft, scratch)
{
	_vString = Structure_vsh();
	_fString = Structure_fsh();
	_ico = new Icosahedron();
	_ico->setColour(1., 0., 0);
	_distance = 0;
}

void DistortMap::addIcosahedron()
{
	for (size_t i = 0; i < _ico->indexCount(); i++)
	{
		_indices.push_back(_ico->index(i) + vertexCount());
	}

	for (size_t i = 0; i < _ico->vertexCount(); i++)
	{
		_vertices.push_back(_ico->vertex(i));
	}
}

void DistortMap::makeKeypoints(double d)
{
	bool shift_x = false;
	bool shift_xy = false;
	std::vector<double> uc = getUnitCell();
	vec3 o = origin();

	for (size_t z = 0; z < uc[2]; z += d)
	{
		for (size_t y = 0; y < uc[1]; y += d)
		{
			for (size_t x = 0; x < uc[0]; x += d)
			{
				vec3 pos = make_vec3(x, y, z);
				vec3_add_to_vec3(&pos, o);
				
				Keypoint kp;
				kp.start = pos;
				kp.shift = empty_vec3();

				kp.vertex = vertexCount();
				_ico->setPosition(pos);
				addIcosahedron();
				
				_keypoints.push_back(kp);
			}
			
			shift_x = !shift_x;
		}

		shift_x = !shift_x;
	}
	
	std::cout << "Number of keypoints: " << _keypoints.size() << std::endl;

	setColour(_r, _g, _b);
	_nnMap.resize(nn());
	
	_distance = d;
	
	for (int z = 0; z < nz(); z++)
	{
		for (int y = 0; y < ny(); y++)
		{
			for (int x = 0; x < nx(); x++)
			{
				vec3 v = make_vec3(x, y, z);
				mat3x3_mult_vec(getRealBasis(), &v);
				vec3_add_to_vec3(&v, o);
				closestPoints(v);
				long ele = element(x, y, z);
				_nnMap[ele] = _closest;
			}
		}
	}
}

void DistortMap::closestPoints(vec3 real)
{
	_closest.clear();

	for (size_t i = 0; i < _keypoints.size(); i++)
	{
		vec3 tmp = vec3_subtract_vec3(real, _keypoints[i].start);
		
		if (vec3_length(tmp) < _distance)
		{
			_closest.push_back(&_keypoints[i]);
		}
	}
}

double DistortMap::cubic_interpolate(vec3 vox000, size_t im)
{
	if (_keypoints.size() == 0)
	{
		return VagFFT::cubic_interpolate(vox000, im);
	}

	collapse(&vox000.x, &vox000.y, &vox000.z);
	int ele = element(vox000.x, vox000.y, vox000.z);
	if (ele >= _nnMap.size())
	{
		std::cout << "Ele over nnMap!" << std::endl;
	}
	std::vector<Keypoint *> points = _nnMap[ele];
	if (points.size() == 0)
	{
		return VagFFT::cubic_interpolate(vox000, im);
	}
	
	vec3 real = vox000;
	mat3x3_mult_vec(getRealBasis(), &real);
	vec3 o = origin();
	vec3_add_to_vec3(&real, o);
	
	double weights = 0;
	vec3 total_motion = empty_vec3();

	for (size_t i = 0; i < points.size(); i++)
	{
		vec3 tmp = vec3_subtract_vec3(points[i]->start, real);
		double weight = (_distance - vec3_length(tmp));
		if (weight < 0) weight = 0;
		
		vec3 sh = points[i]->shift;
		vec3_mult(&sh, weight);
		vec3_add_to_vec3(&total_motion, sh);
		weights += weight;
	}
	
	if (weights <= 0)
	{
		return VagFFT::cubic_interpolate(vox000, im);
	}

	vec3_mult(&total_motion, 1 / weights);
	vec3_add_to_vec3(&total_motion, real);
	vec3_subtract_from_vec3(&total_motion, o);
	mat3x3_mult_vec(getRecipBasis(), &total_motion);

	return VagFFT::cubic_interpolate(total_motion, im);
}

void DistortMap::get_limits(vec3 cur, double dist, vec3 *min, vec3 *max)
{
	double scale = 1;
	vec3 padding = make_vec3(dist/scale, dist/scale, dist/scale);
	*max = vec3_add_vec3(cur, padding);
	*min = vec3_subtract_vec3(cur, padding);
	mat3x3_mult_vec(getRecipBasis(), min);
	mat3x3_mult_vec(getRecipBasis(), max);
}

double DistortMap::aveSigma()
{
	double mean, sigma;
	meanSigma(&mean, &sigma);

	vec3 min, max;
	get_limits(_current, _distance, &min, &max);
	double total = 0;
	double count = 0;
	
	for (int k = min.z; k < max.z; k++)
	{
		for (int j = min.y; j < max.y; j++)
		{
			for (int i = min.x; i < max.x; i++)
			{
				long ele = element(i, j, k);
				total += (getReal(ele) - mean) / sigma;
				count++;
			}
		}
	}
	
	return total / count;
}

double DistortMap::sscore()
{
	if (!_reference)
	{
		return 0;
	}
	
	vec3 o = origin();
	vec3 ro = _reference->origin();
	mat3x3 recipBasis = _reference->getRecipBasis();
	CorrelData cd = empty_CD();
	
	vec3 min, max;
	get_limits(_current, _distance, &min, &max);

	for (int k = min.z; k < max.z; k++)
	{
		for (int j = min.y; j < max.y; j++)
		{
			for (int i = min.x; i < max.x; i++)
			{
				vec3 ijk = make_vec3(i, j, k); /* in Voxels */
				double r0 = cubic_interpolate(ijk, 0);

				mat3x3_mult_vec(getRealBasis(), &ijk); /* to Angstroms */
				vec3_add_to_vec3(&ijk, o); /* in Angstroms */
				vec3_subtract_from_vec3(&ijk, ro); /*in Angstroms*/
				mat3x3_mult_vec(recipBasis, &ijk); /* to Voxels */
				double r1 = _reference->cubic_interpolate(ijk, 0);

				add_to_CD(&cd, r0, r1);
			}
		}
	}

	return -evaluate_CD(cd);
}

void DistortMap::refineKeypoint(int i)
{
	_current = _keypoints[i].start;
	
	double ave = aveSigma();
	if (ave < 0.0)
	{
		std::cout << "Skipping keypoint " << i << 
		" for lack of density..." << std::endl;
		return;
	}

	NelderMeadPtr neld = NelderMeadPtr(new RefinementNelderMead());
	neld->setVerbose(true);
	neld->setCycles(20);
	neld->setEvaluationFunction(DistortMap::score, this);
	neld->setJobName("distort_" + vec3_desc(_keypoints[i].start));

	AnyPtr ax = AnyPtr(new Any(&_keypoints[i].shift.x));
	neld->addParameter(&*ax, Any::get, Any::set, 5, 0.1);
	AnyPtr ay = AnyPtr(new Any(&_keypoints[i].shift.y));
	neld->addParameter(&*ay, Any::get, Any::set, 5, 0.1);
	AnyPtr az = AnyPtr(new Any(&_keypoints[i].shift.z));
	neld->addParameter(&*az, Any::get, Any::set, 5, 0.1);

	neld->refine();
}

void DistortMap::refineKeypoints(VagFFTPtr ref)
{
	std::cout << "Refining keypoints" << std::endl;
	
	if (_keypoints.size() == 0)
	{
		makeKeypoints(50);
	}

	_reference = ref;
	std::cout << "nns: " << nn() << " " << _nnMap.size() << std::endl;

	for (size_t i = 0; i < _keypoints.size(); i++)
	{
		refineKeypoint(i);
	}
}

void DistortMap::writeMRC(std::string filename)
{
	std::ofstream file;
	file.open(filename);
	
	int val = nx();
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = ny();
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = ny();
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 2;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 0;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 0;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 0;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = nx();
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = ny();
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = nz();
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	float a, b, c, al, be, ga;
	a = getUnitCell()[0];
	b = getUnitCell()[1];
	c = getUnitCell()[2];
	al = getUnitCell()[3];
	be = getUnitCell()[4];
	ga = getUnitCell()[5];
	file.write(reinterpret_cast<char *>(&a), sizeof(float));
	file.write(reinterpret_cast<char *>(&b), sizeof(float));
	file.write(reinterpret_cast<char *>(&c), sizeof(float));
	file.write(reinterpret_cast<char *>(&al), sizeof(float));
	file.write(reinterpret_cast<char *>(&be), sizeof(float));
	file.write(reinterpret_cast<char *>(&ga), sizeof(float));

	val = 1;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 2;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 3;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));

	float min = FLT_MAX;
	float max = -FLT_MAX;
	for (size_t i = 0; i < nn(); i++)
	{
		double r = getReal(i);
		if (r > max) max = r;
		if (r < min) min = r;
	}
	
	double mean, sigma;
	meanSigma(&mean, &sigma);
	
	file.write(reinterpret_cast<char *>(&min), sizeof(float));
	file.write(reinterpret_cast<char *>(&max), sizeof(float));
	file.write(reinterpret_cast<char *>(&mean), sizeof(float));
	val = 1;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 0;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	
	val = 0;
	for (size_t i = 0; i < 25; i++)
	{
		file.write(reinterpret_cast<char *>(&val), sizeof(int));
	}

	val = 0;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	file.write("MAP ", sizeof(char) * 4);
	char fourfour = 0x44;
	char null = 0x0;
	file.write(reinterpret_cast<char *>(&fourfour), sizeof(char ));
	file.write(reinterpret_cast<char *>(&fourfour), sizeof(char ));
	file.write(reinterpret_cast<char *>(&null), sizeof(char ));
	file.write(reinterpret_cast<char *>(&null), sizeof(char ));
	file.write(reinterpret_cast<char *>(&sigma), sizeof(float));
	val = 0;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));

	null = ' ';
	for (size_t i = 0; i < 800; i++)
	{
		file.write(reinterpret_cast<char *>(&null), sizeof(char));
	}

	float *tempData = (float *)malloc(sizeof(float) * nn());
	for (size_t i = 0; i < nn(); i++)
	{
		tempData[i] = getReal(i);
	}

	file.write(reinterpret_cast<char *>(tempData), sizeof(float) * nn());
	
	file.close();

}
