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
#include <libsrc/FFT.h>
#include "MRCin.h"
#include "MRCImage.h"
#include "libccp4/ccp4_spg.h"

MRCin::MRCin(std::string filename, bool volume)
{
	_filename = filename;
	_success = (file_exists(_filename));
	_volume = volume;
	
	if (!_success)
	{
		return;
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
	
	float a, b, c;
	file.read(reinterpret_cast<char *>(&a), sizeof(float));
	file.read(reinterpret_cast<char *>(&b), sizeof(float));
	file.read(reinterpret_cast<char *>(&c), sizeof(float));
	
	if (a <= 1e-6) a = 1;
	if (b <= 1e-6) b = 1;
	if (c <= 1e-6) c = 1;
	
	_a = a; _b = b; _c = c;

	std::cout << "Cell edges (Å): " << a << " " << b << " " 
	<< c << std::endl;
	
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

VagFFTPtr MRCin::getVolume()
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
	
	int dx = _nx / 3;
	int dy = _ny / 3;
	int dz = _nz / 3;

	fft->setUnitCell(uc);
	fft->setStatus(FFTRealSpace);
//	fft->makePlans();
	
	for (size_t i = 0; i < _values.size(); i++)
	{
		fft->setElement(i, _values[i], 0);
	}
	
	VagFFTPtr sub = fft->subFFT(dx, 2 * dx, dy, 2 * dy, dz, 2 * dz);
	sub->setStatus(FFTRealSpace);
	
	return sub;
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

