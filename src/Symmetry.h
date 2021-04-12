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

#ifndef __em__Symmetry__
#define __em__Symmetry__

#include <string>
#include <vector>
#include <hcsrc/mat3x3.h>

class Symmetry
{
public:
	Symmetry(std::string filename);
	
	size_t matrixCount()
	{
		return _matrices.size();
	}

	const mat3x3& matrix(int i)
	{
		return _matrices[i];
	}

private:
	void getMatrices();
	mat3x3 getNextMatrix(std::string &contents, char **pos);

	std::string _filename;
	std::vector<mat3x3> _matrices;

};

#endif
