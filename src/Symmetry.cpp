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

#include "Symmetry.h"
#include <hcsrc/FileReader.h>

Symmetry::Symmetry(std::string filename)
{
	_filename = filename;
	getMatrices();
}

mat3x3 Symmetry::getNextMatrix(std::string &contents, char **pos)
{
	mat3x3 m = make_mat3x3();
	char *ch = *pos;
	int num = 0;

	for (size_t i = 0; i < 9; i++)
	{
		char *next = NULL;
		double val = strtod(ch, &next);
		m.vals[num] = val;
		num++;
		ch = next + 1;
		
		while (*ch == ',' || *ch == ' ' || *ch == '\n')
		{
			ch++;
		}
		
		if (*ch == ')' || num == 9)
		{
			if (num < 9)
			{
				std::cout << "Warning: not enough entries for " <<
				" matrix in " << _filename << "!" << std::endl;
			}
			break;
		}
	}
	
	*pos = ch;

	return m;
}

void Symmetry::getMatrices()
{
	std::string contents = get_file_contents(_filename);
	
	size_t pos = contents.find('(', 0) + 1;
	char *ch = &contents[pos];
	
	while (*ch != '\0')
	{
		mat3x3 next = getNextMatrix(contents, &ch);
		_matrices.push_back(next);
		
		while (*ch != '(')
		{
			ch++;
			
			if (*ch == '\0')
			{
				return;
			}
		}

		ch++;
	}
}
