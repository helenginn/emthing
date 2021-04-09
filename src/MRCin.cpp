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

#include <QImage>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <hcsrc/FileReader.h>
#include "DistortMap.h"
#include "Control.h"
#include "MRCin.h"
#include "MRCImage.h"
#include "libccp4/ccp4_spg.h"

MRCin::MRCin(std::string filename, bool volume)
{
	_filename = filename;
	_success = (file_exists(_filename));
	std::string auxfile = getBaseFilenameWithPath(filename) + ".aux";
	_aux = (file_exists(auxfile) ? auxfile : "");

	_volume = volume;
	
	if (!_success)
	{
		std::cout << "File " << filename << " does not exist" << std::endl;
		exit(0);
	}
	
	process();
}

void MRCin::process()
{
	std::ifstream file;
	file.open(_filename);
	
	file.read(reinterpret_cast<char *>(&_nx), sizeof(int));
	file.read(reinterpret_cast<char *>(&_ny), sizeof(int));
	file.read(reinterpret_cast<char *>(&_nz), sizeof(int));

	std::cout << "Size: " << _nx << " " << _ny << " " << _nz << std::endl;

	file.read(reinterpret_cast<char *>(&_mode), sizeof(int));
	
	std::cout << "Mode: ";

	switch (_mode)
	{
		case 0:
		std::cout << "8-bit signed integer" << std::endl;
		break;

		case 1:
		std::cout << "16-bit signed integer" << std::endl;
		break;

		case 2:
		std::cout << "32-bit signed floating point" << std::endl;
		break;

		case 3:
		std::cout << "complex 16-bit integers" << std::endl;
		break;

		case 4:
		std::cout << "complex 32-bit reals" << std::endl;
		break;

		case 6:
		std::cout << "16-bit unsigned integers" << std::endl;
		break;
		
		default:
		std::cout << "Unknown" << std::endl;
		break;
	}

	file.seekg(28);
	int sx, sy, sz;
	file.read(reinterpret_cast<char *>(&sx), sizeof(int));
	file.read(reinterpret_cast<char *>(&sy), sizeof(int));
	file.read(reinterpret_cast<char *>(&sz), sizeof(int));
	
	float a, b, c;
	file.read(reinterpret_cast<char *>(&a), sizeof(float));
	file.read(reinterpret_cast<char *>(&b), sizeof(float));
	file.read(reinterpret_cast<char *>(&c), sizeof(float));
	
	std::cout << "Sampling: " << sx << " " << sy << " " << sz << std::endl;
	_a = a / (float)sx; _b = b / (float)sy; _c = c / (float)sz;

	std::cout << "Cell edges (Å): " << _a << " " << _b << " " 
	<< _c << std::endl;

	file.seekg(196);
	file.read(reinterpret_cast<char *>(&_ori[0]), sizeof(float) * 3);
	
	file.seekg(208);
	char filetype[5];
	file.read(filetype, sizeof(char) * 4);
	filetype[4] = '\0';
	std::cout << "File type: " << filetype << std::endl;
	
	file.seekg(104);
	char extyp[5];
	file.read(extyp, sizeof(char) * 4);
	extyp[4] = '\0';
	if (extyp[0] != '\0')
	{
		std::cout << "Extended header type: " << extyp << std::endl;
	}
	else
	{
		std::cout << "No extended header type" << std::endl;
	}

	file.seekg(1024);
	std::cout << "Reading data..." << std::endl;
	
	if (_mode != 2)
	{
		std::cout << "Unsupported type" << std::endl;
		_success = false;
		return;
	}
	
	if (!_volume)
	{
		prepareDataForImages();
	}

	_nn = _nx * _ny * _nz;
	_values.resize(_nn);
	file.read(reinterpret_cast<char *>(&_values[0]), _nn * sizeof(float));

	file.close();
	std::cout << "Read data." << std::endl;
}

void MRCin::prepareDataForImages()
{
	if (width() % 4 != 0)
	{
		int extra = 4 - (width() % 4);
		int tw = width() + extra;

		std::vector<float> tmp;
		tmp.resize(tw * _ny * _nz);
		
		for (long i = 0; i < _ny * _nz; i++)
		{
			memcpy(&tmp[i * tw], &_values[i * _nx], sizeof(float) * _nx);
			float last = _values[(i + 1) * _nx - 1];
			tmp[(i + 1) * tw - 2] = last;
			tmp[(i + 1) * tw - 1] = last;
		}
		
		_values = tmp;
		_nx = tw;
	}
}

void MRCin::cropLimits(VagFFTPtr o, vec3 *min, vec3 *max)
{
	*max = empty_vec3();
	*min = make_vec3(o->nx(), o->ny(), o->nz());
	
	double mean, sigma;
	o->meanSigma(&mean, &sigma);

	for (int z = 0; z < o->nz(); z++)
	{
		for (int y = 0; y < o->ny(); y++)
		{
			for (int x = 0; x < o->nx(); x++)
			{
				float real = o->getReal(x, y, z);
				real = (real - mean) / sigma;
				
				if (real > 4)
				{
					if (min->x > x) min->x = x;
					if (min->y > y) min->y = y;
					if (min->z > z) min->z = z;
					if (max->x < x) max->x = x;
					if (max->y < y) max->y = y;
					if (max->z < z) max->z = z;
				}
			}
		}
	}
}

DistortMapPtr MRCin::getVolume()
{
	VagFFTPtr fft = VagFFTPtr(new VagFFT(_nx, _ny, _nz));
	CSym::CCP4SPG *spg = CSym::ccp4spg_load_by_ccp4_num(1);
	fft->setSpaceGroup(spg);

	std::vector<double> uc;
	uc.push_back(_a * _nx);
	uc.push_back(_b * _ny);
	uc.push_back(_c * _nz);
	uc.push_back(90);
	uc.push_back(90);
	uc.push_back(90);

	fft->setUnitCell(uc);
	fft->setStatus(FFTRealSpace);
	
	bool plan = Control::valueForKey("make-plans") == "true";
	
	if (_aux.length() && plan)
	{
		fft->makePlans();
	}

	for (size_t i = 0; i < _values.size(); i++)
	{
		fft->setElement(i, _values[i], 0);
	}
	
	vec3 ori = make_vec3(_ori[0], _ori[1], _ori[2]);
	fft->setOrigin(ori);
	
	double fraction = atof(Control::valueForKey("crop-fraction").c_str());
	vec3 min, max;
	
	if (_aux.length())
	{
		std::cout << "Auxiliary file found. Not cropping map..." << std::endl;
		std::cout << "Cube length: " << fft->getCubicScale() / fft->nx() <<  
		std::endl;
		DistortMapPtr dfft = DistortMapPtr(new DistortMap(*fft));
		dfft->setAuxiliary(_aux);
		dfft->setFilename(_filename);
		return dfft;
	}
	if (fraction < 1e-6 || fraction > 1.00001)
	{
		std::cout << "Cropping image: ";
		std::cout << "determining crop limits automatically..." << std::endl;
		cropLimits(fft, &min, &max);
		vec3 diff = make_vec3(30, 30, 30);
		vec3_subtract_from_vec3(&min, diff);
		vec3_add_to_vec3(&max, diff);
	}
	else
	{
		std::cout << "Cropping image: ";
		std::cout << "using fraction " << fraction << " ... " << std::endl;
		vec3 size = make_vec3(fft->nx(), fft->ny(), fft->nz());
		vec3 middle = size;
		vec3_mult(&middle, 0.5);
		vec3 keep = middle;
		vec3_mult(&keep, fraction);
		min = vec3_subtract_vec3(middle, keep);
		max = vec3_add_vec3(middle, keep);
	}

	std::cout << "Minimum: " << vec3_desc(min) << std::endl;
	std::cout << "Maximum: " << vec3_desc(max) << std::endl;

	std::cout << "From " << min.x << " " << min.y << " " << min.z << " to "
	<< max.x << " " << max.y << " " << max.z << std::endl;
	
	VagFFTPtr sub = fft->subFFT(min.x, max.x, min.y, max.y, min.z, max.z);
	sub->setStatus(FFTRealSpace);
	std::cout << "Cube length: " << sub->getCubicScale() / sub->nx() <<  std::endl;
	DistortMapPtr dfft = DistortMapPtr(new DistortMap(*sub));
	dfft->setFilename(_filename);
	return dfft;
}

MRCImage *MRCin::image(int i)
{
	if (_imageMap.count(i))
	{
		return _imageMap[i];
	}

	float *data = imagePtr(i);
	
	MRCImage *im = new MRCImage(width(), height());
	std::ostringstream str;
	str << std::setfill('0') << std::setw(5) << i_to_str(i);
	std::string name = "x" + str.str();
	im->setName(name + "_" + getBaseFilename(_filename));
	im->setRawData(data);
	im->estimateRange();
	im->redraw();
//	im->bin(2);
//	im->bin(2);
//	im->bin(2);
//	im->reverse();
	im->blur();
	
	_imageMap[i] = im;

	return im;
}
