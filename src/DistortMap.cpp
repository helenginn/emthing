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
#include <hcsrc/FileReader.h>
#include <libsrc/AtomGroup.h>
#include <libsrc/Atom.h>
#include <hcsrc/Any.h>
#include <iostream>
#include <fstream>
#include <fftw3.h>

void DistortMap::initialise()
{
	_threshold = -2.0;
	_vString = Structure_vsh();
	_fString = Structure_fsh();
	_ico = new Icosahedron();
	_ico->setColour(1., 0., 0);
	_distance = 50;
	_focus = false;
	_current = empty_vec3();
	_r = 0; _g = 0; _b = 0;
}

DistortMap::DistortMap(int nx, int ny, int nz, int nele, int scratches)
: VagFFT(nx, ny, nz, nele, scratches)
{
	initialise();
}

DistortMap::DistortMap(VagFFT &fft, int scratch) : VagFFT(fft, scratch)
{
	initialise();
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
				kp.density = 0;
				kp.correlation = 0;

				kp.vertex = vertexCount();
				_ico->setPosition(pos);
				addIcosahedron();
				
				_keypoints.push_back(kp);
			}
			
			shift_x = !shift_x;
		}

		shift_x = !shift_x;
	}
	
	prepareKeypoints();
	
	_distance = d;
}

void DistortMap::prepareKeypoints()	
{
	std::cout << "Number of keypoints: " << _keypoints.size() << std::endl;

	vec3 o = origin();
	setColour(_r, _g, _b);
	_nnMap.resize(nn());
	
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

vec3 DistortMap::motion(vec3 real, std::vector<Keypoint *> &points,
                        double *weights)
{
	*weights = 0;
	vec3 total_motion = empty_vec3();

	for (size_t i = 0; i < points.size(); i++)
	{
		vec3 tmp = vec3_subtract_vec3(points[i]->start, real);
		double weight = (_distance - vec3_length(tmp));
		if (weight < 0) weight = 0;
		
		vec3 sh = points[i]->shift;
		vec3_mult(&sh, weight);
		vec3_add_to_vec3(&total_motion, sh);
		*weights += weight;
	}

	vec3_mult(&total_motion, 1 / *weights);
	return total_motion;
}

double DistortMap::cubic_interpolate(vec3 vox000, size_t im)
{
	if ((vox000.x < 0 || vox000.y < 0 || vox000.z < 0) &&
	    _status == FFTRealSpace)
	{
		return 0;
	}

	if ((vox000.x >= nx() || vox000.y >= ny() || vox000.z >= nz()) &&
	    _status == FFTRealSpace)
	{
		return 0;
	}

	if (_keypoints.size() == 0)
	{
		return VagFFT::cubic_interpolate(vox000, im);
	}

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
	vec3 total_motion = motion(real, points, &weights);
	
	if (weights <= 0)
	{
		return VagFFT::cubic_interpolate(vox000, im);
	}

	vec3_add_to_vec3(&total_motion, real);
	vec3_subtract_from_vec3(&total_motion, o);
	mat3x3_mult_vec(getRecipBasis(), &total_motion);

	return VagFFT::cubic_interpolate(total_motion, im);
}

double DistortMap::correlateKeypoints(DistortMapPtr other)
{
	double dots = 0;
	double xs = 0; double ys = 0;
	
	for (size_t i = 0; i < _keypoints.size(); i++)
	{
		vec3 pos = _keypoints[i].start;
		
		double density = _keypoints[i].density;
		if (density <= 0)
		{
			continue;
		}

		closestPoints(pos);
		std::vector<Keypoint *> myPoints = _closest;
		if (myPoints.size() == 0)
		{
			continue;
		}

		other->closestPoints(pos);
		std::vector<Keypoint *> points = other->_closest;
		
		if (points.size() == 0)
		{
			continue;
		}
		
		double my_weight = 0;
		double other_weight = 0;

		vec3 my_mot = motion(pos, myPoints, &my_weight);

		vec3 other_mot = other->motion(pos, points, &other_weight);
		
		double correl = _keypoints[i].correlation;

//		vec3_mult(&my_mot, density);
//		vec3_mult(&other_mot, density);
		
		double dot = vec3_dot_vec3(my_mot, other_mot);
		dots += dot;
		xs += vec3_sqlength(my_mot);
		ys += vec3_sqlength(other_mot);
	}
	
	double cc = dots / (sqrt(xs) * sqrt(ys));
	return cc;
}

void DistortMap::get_limits(vec3 cur, double dist, vec3 *min, vec3 *max)
{
	double scale = 2.0;
	vec3 padding = make_vec3(dist/scale, dist/scale, dist/scale);
	vec3 no_orig = vec3_subtract_vec3(cur, _origin);
	*max = vec3_add_vec3(no_orig, padding);
	*min = vec3_subtract_vec3(no_orig, padding);
	mat3x3_mult_vec(getRecipBasis(), min);
	mat3x3_mult_vec(getRecipBasis(), max);
}

double DistortMap::aveSigma(bool ref)
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
				mat3x3_mult_vec(recipBasis, &ijk); /* to other Voxels */
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
	_reference->_current = _current;
	_keypoints[i].density = ave;
//	double rave = _reference->aveSigma();

	if (ave < 0.0)
	{
		std::cout << "Skipping keypoint " << i << 
		" for lack of density..." << std::endl;
		return;
	}
	
	std::cout << "Refining keypoint " << i << " (average sigma level " <<
	ave << ")" << std::endl;

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

	_keypoints[i].correlation = -sscore();
}

void DistortMap::refineKeypoints(DistortMapPtr ref)
{
	std::cout << "Refining keypoints" << std::endl;
	
	if (_keypoints.size() == 0)
	{
		makeKeypoints(50);
	}

	_reference = ref;
	std::cout << "nns: " << nn() << " " << _nnMap.size() << std::endl;
	std::cout << _keypoints.size() << " keypoints for " << _filename << "." << std::endl;

	for (size_t n = 0; n < 2; n++)
	{
		for (size_t i = 0; i < _keypoints.size(); i++)
		{
			refineKeypoint(i);
		}
	}
}

void DistortMap::writeAuxiliary(std::string filename)
{
	std::string auxfile = getBaseFilenameWithPath(filename) + ".aux";
	std::ofstream file;
	file.open(auxfile);
	
	int val = _keypoints.size();
	file.write(reinterpret_cast<char *>(&val), sizeof(int));

	vec3 o = origin();
	file.write(reinterpret_cast<char *>(&o), sizeof(vec3));

	file.write(reinterpret_cast<char *>(&_distance), sizeof(double));

	file.write(reinterpret_cast<char *>(&_keypoints[0]), 
	           sizeof(Keypoint) * _keypoints.size());
	
	file.close();
}

void DistortMap::writeMRC(std::string filename, bool withDistortion)
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
		
		if (withDistortion)
		{
			vec3 frac = fracFromElement(i);
			r = getCompFromFrac(frac, 0);
		}

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
	file.write(reinterpret_cast<char *>(&val), sizeof(int)); // byte 25
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 20140;
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
	val = 0;
	
	for (size_t i = 0; i < 21; i++)
	{
		file.write(reinterpret_cast<char *>(&val), sizeof(int));
	}

	float origin[3];
	origin[0] = _origin.x;
	origin[1] = _origin.y;
	origin[2] = _origin.z;

	file.write(reinterpret_cast<char *>(origin), sizeof(float) * 3);

	val = 0;
	file.write("MAP ", sizeof(char) * 4);
	char fourfour = 0x44;
	char null = 0x0;
	file.write(reinterpret_cast<char *>(&fourfour), sizeof(char ));
	fourfour = 0x41;
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
		double r = getReal(i);
		
		if (withDistortion)
		{
			vec3 frac = fracFromElement(i);
			r = getCompFromFrac(frac, 0);
		}

		tempData[i] = r;
	}

	file.write(reinterpret_cast<char *>(tempData), sizeof(float) * nn());
	
	if (!withDistortion)
	{
		writeAuxiliary(filename);
	}
	
	std::cout << "Written file " << (withDistortion ? "with" : "") << 
	" distortion." << std::endl;
	
	file.close();

}

void DistortMap::loadFromAuxiliary()
{
	if (_aux.length() == 0) return;

	std::ifstream file;
	file.open(_aux);
	
	int nKeypoints;
	file.read(reinterpret_cast<char *>(&nKeypoints), sizeof(int));

	vec3 o = empty_vec3();
	file.read(reinterpret_cast<char *>(&o), sizeof(vec3));
	setOrigin(o);

	file.read(reinterpret_cast<char *>(&_distance), sizeof(double));

	_keypoints.resize(nKeypoints);
	file.read(reinterpret_cast<char *>(&_keypoints[0]), 
	          sizeof(Keypoint) * nKeypoints);

	file.close();

//	prepareKeypoints();
	std::cout << "Loaded keypoints from auxiliary." << std::endl;
}

void DistortMap::dropData()
{
	free(_data);
	_data = NULL;
}

void DistortMap::maskWithAtoms(AtomGroupPtr atoms)
{
	_pdbMask.resize(nn());

	for (size_t i = 0; i < atoms->atomCount(); i++)
	{
		AtomPtr a = atoms->atom(i);
		vec3 pos = a->getAbsolutePosition();
		vec3_subtract_from_vec3(&pos, _origin);
		mat3x3_mult_vec(getRealBasis(), &pos);
		
		const int w = 10;

		for (int z = -w; z < w; z++)
		{
			for (int y = -w; y < w; y++)
			{
				for (int x = -w; x < w; x++)
				{
					vec3 tmp = make_vec3(x + pos.x, y + pos.y, z + pos.z);
					collapse(&tmp.x, &tmp.y, &tmp.z);
					int ele = element(tmp.x, tmp.y, tmp.z);
					_pdbMask[ele] = 1;
				}
			}
		}
	}
}

double DistortMap::rotateRoundCentre(mat3x3 rotation, vec3 add, 
                                     DistortMapPtr other, double scale, 
                                     bool write, bool useCentroid)
{
	fftwf_complex *tempData;

	if (write)
	{
		tempData = (fftwf_complex *)fftwf_malloc(_nn * sizeof(FFTW_DATA_TYPE));
		memset(tempData, 0, sizeof(FFTW_DATA_TYPE) * _nn);
	}
	mat3x3 transpose = mat3x3_transpose(rotation);
	vec3 centre = make_vec3(0.5, 0.5, 0.5);
	
	if (useCentroid)
	{
		centre = fractionalCentroid();
	}

	CorrelData cd = empty_CD();
	double mean, sigma;
	meanSigma(&mean, &sigma);
	
	for (int k = 0; k < _nz; k++)
	{
		for (int j = 0; j < _ny; j++)
		{
			for (int i = 0; i < _nx; i++)
			{
				long end = element(i, j, k);
				
				double r = getReal(end);
				if ((r - mean) / sigma < _threshold && !write)
				{
					continue;
				}

				vec3 ijk = make_vec3(i, j, k); /* in Voxels */
				mat3x3_mult_vec(_realBasis, &ijk); /* to Angstroms */
				vec3 orig = ijk;

				if (other)
				{
					vec3_add_to_vec3(&orig, _origin); /* in Angstroms */
					vec3_subtract_from_vec3(&orig, other->_origin); /*in Angstroms*/
					mat3x3_mult_vec(other->_recipBasis, &orig); /* to Voxels */
					collapse(&orig.x, &orig.y, &orig.z);
					
					int ele = other->element(orig.x, orig.y, orig.z);
					if (!write && _focus && other->_pdbMask.size() && 
					    other->_pdbMask[ele] == 0)
					{
						continue;
					}
				}

				ijk = make_vec3(i, j, k); /* in Voxels */
				mat3x3_mult_vec(_voxelToFrac, &ijk); /* to fractional */

				vec3_subtract_from_vec3(&ijk, centre); /* in fractional */

				mat3x3_mult_vec(transpose, &ijk); /* in fractional */
				vec3_mult(&ijk, scale); /* unit-independent scale */
				vec3_add_to_vec3(&ijk, centre); /* in fractional */

				mat3x3_mult_vec(_toReal, &ijk); /* to Angstroms */

				vec3_subtract_from_vec3(&ijk, add); /* in Angstroms */
				mat3x3_mult_vec(_recipBasis, &ijk); /* to Voxels */

				double r0 = cubic_interpolate(ijk, 0);
				
				if (other)
				{
					double r1 = other->cubic_interpolate(orig, 0);
					add_to_CD(&cd, r0, r1);
				}
				
				if (write)
				{
					tempData[end][0] = r0;
					tempData[end][1] = cubic_interpolate(ijk, 1);
				}
			}
		}
	}
	
	if (!write)
	{
		return evaluate_CD(cd);
	}

	/* Loop through and convert data into amplitude and phase */
	for (int n = 0; n < _nn; n++)
	{
		long m = finalIndex(n);
		_data[m][0] = tempData[n][0];
		_data[m][1] = tempData[n][1];
	}

	free(tempData);

	return evaluate_CD(cd);
}

DistortMapPtr DistortMap::binned(int bin)
{
	if (false && bin > 0 && (_nx % bin != 0 || _ny % bin != 0 || _nz % bin != 0)) 
	{
		std::cout << "Warning: binning " << bin << "not exactly "\
		" compatible with dimensions " << _nx << " " << _ny << " " << _nz
		<< std::endl;
	}

	int mx = _nx / bin;
	int my = _ny / bin;
	int mz = _nz / bin;

	float divide = bin * bin * bin;

	DistortMapPtr newMap = DistortMapPtr(new DistortMap(mx, my, mz, 0, 0));
	newMap->setOrigin(origin());
	
	for (size_t z = 0; z < nz(); z += bin)
	{
		for (size_t y = 0; y < ny(); y += bin)
		{
			for (size_t x = 0; x < nx(); x += bin)
			{
				float total = 0;

				for (size_t k = 0; k < bin; k++)
				{
					for (size_t j = 0; j < bin; j++)
					{
						for (size_t i = 0; i < bin; i++)
						{
							float value = getReal(x + i, y + j, z + k);
							total += value;
						}
					}
				}
				
				total /= divide;
				
				int element = newMap->element(x / bin, y / bin, z / bin);
				newMap->setReal(element, total);
			}
		}
	}
	
	double vox = (bin - 1) / 2;
	vec3 hs = make_vec3(vox, vox, vox);
	newMap->setUnitCell(getUnitCell());
	newMap->setStatus(FFTRealSpace);

	mat3x3_mult_vec(newMap->getRealBasis(), &hs);
	vec3_mult(&hs, +1);
	newMap->addToOrigin(hs);
	
	return newMap;
}

int DistortMap::bestBin(float target)
{
	double dim = getCubicScale() / (float)_nx;
	double smallest = FLT_MAX;
	int bin = 1;
	
	for (size_t i = 0; i < 5; i++)
	{
		double diff = fabs(dim * (1 << i) - target);
		
		if (diff < smallest)
		{
			smallest = diff;
			bin = (1 << i);
		}
	}
	
	return bin;
}
