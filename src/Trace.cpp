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

#include "Trace.h"
#include <cfloat>
#include <QThread>
#include <iostream>
#include <hcsrc/Fibonacci.h>
#include <libsrc/FFT.h>
#include <h3dsrc/shaders/vStructure.h>
#include <h3dsrc/shaders/fStructure.h>

#define DISTANCE (6)

Trace::Trace() : SlipObject()
{
	_renderType = GL_LINES;
	_vString = Structure_vsh();
	_fString = Structure_fsh();
}

void Trace::makeTrials()
{
	Fibonacci f;
	double dist = DISTANCE;
	f.generateLattice(161, dist);
	_trials = f.getPoints();
	std::cout << "Made trial points" << std::endl;
}

double Trace::score(vec3 dir, vec3 last, vec3 trial, bool first)
{
	mat3x3 basis = _fft->getRecipBasis();
	bool check = !first;
	if (check)
	{
		vec3_set_length(&trial, 1);
		double dot = vec3_dot_vec3(dir, trial);
		if (dot < 0.)
		{
			return -FLT_MAX;
		}
	}

	vec3 p = vec3_add_vec3(last, trial);
	
	for (int i = 0; i < (int)_vertices.size() - 10; i+=2)
	{
		vec3 v = vec_from_pos(_vertices[i].pos);
		vec3_subtract_from_vec3(&v, p);
		
		if (vec3_length(v) < DISTANCE)
		{
			return -FLT_MAX;
		}
	}
	
	for (int i = 0; i < (int)_bad.size(); i++)
	{
		vec3 v = _bad[i];
		vec3_subtract_from_vec3(&v, p);
		
		if (vec3_length(v) < DISTANCE)
		{
			return -FLT_MAX;
		}
	}

	mat3x3_mult_vec(basis, &p);

	double real = _fft->cubic_interpolate(p);
	double weights = 0;
	
	for (int k = -1; k <= 1; k++)
	{
		for (int j = -1; j <= 1; j++)
		{
			for (int i = -1; i <= 1; i++)
			{
				double weight = (i == 0 && j == 0 && k == 0) ? 1.0 : 0.2;
				weights += weight;
				vec3 px = p;
				px.x += i; px.y += j; px.z += k;
				real += _fft->cubic_interpolate(px) * weight;
			}
		}
	}
	
	real /= weights;

	if (real < _min)
	{
		return -FLT_MAX;
	}

	return real;
}

void Trace::go_back(size_t times)
{
	if (_vertices.size() <= 1)
	{
		return;
	}
	
	if (_vertices.size() > _longestV.size())
	{
		_longestV = _vertices;
		_longestI = _indices;
	}

	for (size_t i = 0; i < 2 * times; i += 2)
	{
		vec3 v = vec_from_pos(_vertices.back().pos);
		_bad.push_back(v);
		_vertices.pop_back();
		_vertices.pop_back();
		_indices.pop_back();
		_indices.pop_back();
	}
	std::cout << "Going back" << std::endl;
}

bool Trace::findNextBest(vec3 *next)
{
	bool first = true;
	vec3 last = vec_from_pos(_vertices[_vertices.size() - 1].pos);
	vec3 dir = empty_vec3();

	if (_vertices.size() > 2)
	{
		vec3 prev = vec_from_pos(_vertices[_vertices.size() - 3].pos);
		dir = vec3_subtract_vec3(last, prev);
		vec3_set_length(&dir, 1);
		first = false;
	}

	
	double best = -FLT_MAX;
	vec3 chosen = empty_vec3();

	for (size_t i = 0; i < _trials.size(); i++)
	{
		double s = score(dir, last, _trials[i], first);

		if (s > best)
		{
			best = s;
			chosen = _trials[i];
		}
	}
	
	if (best == -FLT_MAX)
	{
		return false;
	}

	*next = vec3_add_vec3(last, chosen);
	return true;
}

void Trace::trace()
{
	std::cout << "Tracing" << std::endl;
	makeTrials();
	
	if (vertexCount() == 0)
	{
		lockMutex();
		addVertex(_start);
		unlockMutex();
	}
	
	int max = 5000;
	int streak = 0;
	
	for (size_t i = 0; i < max; i++)
	{
		lockMutex();
		vec3 next;
		bool success = findNextBest(&next);
		int count = 0;
		
		while (!success && count < 10)
		{
			streak = 0;
			count++;
			go_back();
			success = findNextBest(&next);
		}
		
		if (count >= 10)
		{
			break;
		}
		else if (count == 0)
		{
			streak++;
		}

		std::cout << "Found next: " << vec3_desc(next) << std::endl;
		addIndex(-1);
		addVertex(next);
		addIndex(-1);
		addVertex(next);
		unlockMutex();
		QThread::msleep(20);
	}

	_vertices = _longestV;
	_indices = _longestI;
	std::cout << vertexCount() << " vertices." << std::endl;
	unlockMutex();

	resultReady();
}
