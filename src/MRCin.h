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

#ifndef __em__MRCin__
#define __em__MRCin__

class MRCImage;
#include <libsrc/shared_ptrs.h>

class MRCin
{
public:
	MRCin(std::string fn, bool volume = false);

	size_t imageCount()
	{
		return (size_t)_nz;
	}
	
	size_t height()
	{
		return _ny;
	}
	
	size_t width()
	{
		return _nx;
	}
	
	MRCImage *image(int i);
	
	float *const imagePtr(int i)
	{
		return &_values[i * _nx * _ny];
	}
	
	VagFFTPtr getVolume();
private:
	void process();
	void prepareDataForImages();
	std::string _filename;
	std::vector<float> _values;
	std::map<int, MRCImage *> _imageMap;

	int _nx, _ny, _nz;
	int _mode;
	float _a, _b, _c;
	long int _nn;
	bool _success;
	bool _volume;
};

#endif