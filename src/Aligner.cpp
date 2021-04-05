// Aligner
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

#include "Aligner.h"
#include "DistortMap.h"
#include "Control.h"
#include <iostream>
#include <hcsrc/Any.h>
#include <hcsrc/FileReader.h>
#include <hcsrc/RefinementNelderMead.h>

Aligner::Aligner(DistortMapPtr v1, DistortMapPtr v2) : Icosahedron()
{
	_scale = 1;
	_microTrans = empty_vec3();
	_microAngles = empty_vec3();
	_ori0 = v1;
	_ori1 = v2;

	double cor = -_ori1->rotateRoundCentre(make_mat3x3(), empty_vec3(), _ori0); 
	std::cout << "Initial correlation: " << cor << std::endl;

	v1->fft(FFTRealToReciprocal);
	_v0 = subset(v1);
	_v0->switchToAmplitudePhase();
	v1->fft(FFTReciprocalToReal);

	v2->fft(FFTRealToReciprocal);
	_v1 = subset(v2);
	_v1->switchToAmplitudePhase();
	v2->fft(FFTReciprocalToReal);

	triangulate();
	triangulate();
	triangulate();
	resize(50);
	setColour(0.5, 0.5, 0.5);
}

VagFFTPtr Aligner::subset(VagFFTPtr fft)
{
	int nx = 10;
	int ny = 10;
	int nz = 10;

	VagFFTPtr sub = fft->subFFT(-nx, nx, -ny, ny, -nz, nz);
	sub->flipCentre();

	return sub;
}

mat3x3 Aligner::makeRotation(vec3 axis, double deg)
{
	vec3_set_length(&axis, 1);
	vec3 x = make_vec3(1, 0, 0);
	double check = fabs(vec3_dot_vec3(x, axis));
	
	if (abs(check - 1) < 1e-6)
	{
		x = make_vec3(0, 1, 0);
	}

	vec3 cross = vec3_cross_vec3(x, axis);
	mat3x3 basis = mat3x3_rhbasis(x, cross);
	
	mat3x3 rot = mat3x3_unit_vec_rotation(axis, deg2rad(deg));
	mat3x3 tmp = mat3x3_mult_mat3x3(rot, basis);

	return tmp;
}

vec3 colour(double score)
{
	score *= 3;
	if (score < 1)
	{
		return make_vec3(score, 0, 0);
	}
	else if (score < 2)
	{
		return make_vec3(1, score - 1, 0);
	}
	else if (score < 3)
	{
		return make_vec3(1, 1, score - 2);
	}
	
	return make_vec3(1, 1, 1);
}

void Aligner::translation()
{
	_v0 = VagFFTPtr(new VagFFT(*_ori0));
	_v1 = VagFFTPtr(new VagFFT(*_ori1));
	
	VagFFTPtr copy0 = VagFFTPtr(new VagFFT(*_v0));
	
	/* Interpolate in real space in case of differing unit cells */
	for (int z = 0; z < copy0->nz(); z++)
	{
		for (int y = 0; y < copy0->ny(); y++)
		{
			for (int x = 0; x < copy0->nx(); x++)
			{
				vec3 frac = make_vec3(x / (float)copy0->nx(), 
				                      y / (float)copy0->ny(),
				                      z / (float)copy0->nz());

				double real1 = _v1->getCompFromFrac(frac, 0);

				copy0->setReal(x, y, z, real1);
				copy0->setImag(x, y, z, 0);
			}
		}
	}

	_v0->fft(FFTRealToReciprocal);
	copy0->fft(FFTRealToReciprocal);
	
	/* Cross-correlation function */
	for (int z = 0; z < copy0->nz(); z++)
	{
		for (int y = 0; y < copy0->ny(); y++)
		{
			for (int x = 0; x < copy0->nx(); x++)
			{
				double real0 = _v0->getReal(x, y, z);
				double imag0 = _v0->getImag(x, y, z);

				double real1 = copy0->getReal(x, y, z);
				double imag1 = copy0->getReal(x, y, z);
				imag1 *= -1;

				double nr = real0 * real1 - imag0 * imag1;
				double ni = real0 * imag1 + real1 * imag0;

				copy0->setReal(x, y, z, nr);
				copy0->setImag(x, y, z, ni);
			}
		}
	}
	
	copy0->fft(FFTReciprocalToReal);
	
	double best_val = 0;
	vec3 pos = empty_vec3();
	for (int y = 0; y < copy0->nn(); y++)
	{
		double r = copy0->getReal(y);
		if (r > best_val)
		{
			best_val = r;
			pos = copy0->fracFromElement(y);
		}
	}
	
	if (pos.x > 0.5) pos.x -= 1;
	if (pos.y > 0.5) pos.y -= 1;
	if (pos.z > 0.5) pos.z -= 1;

	if (pos.x <= -0.5) pos.x += 1;
	if (pos.y <= -0.5) pos.y += 1;
	if (pos.z <= -0.5) pos.z += 1;

	std::cout << "Best translation (fractional): " << vec3_desc(pos) << std::endl;
	mat3x3 tr = _ori0->toReal();
	mat3x3_mult_vec(tr, &pos);
	_translation = pos;
	
}

void Aligner::rotation()
{
	double bestest = 0;
	mat3x3 b = make_mat3x3();

	for (size_t i = 0; i < _vertices.size(); i++)
	{
		double best = 0;

		vec3 trial = vec_from_pos(_vertices[i].pos);
		for (double j = 0; j < 360; j += 15)
		{
			mat3x3 m = makeRotation(trial, j);
			mat3x3_scale(&m, -1, -1, -1);
			double cor = _v0->compareReciprocal(_v1, m);

			if (best < cor)
			{
				best = cor;
			}

			if (bestest < cor)
			{
				bestest = cor;
				b = m;
			}
		}

		best -= 0.5;
		if (best < 0) best = 0;
		best /= (1 - 0.5);
		best *= best;
		vec3 c = colour(best);
		lockMutex();
		_vertices[i].color[0] = c.x;
		_vertices[i].color[1] = c.y;
		_vertices[i].color[2] = c.z;
		unlockMutex();
	}

	std::cout << "Best orientation: " << std::endl
	 << mat3x3_desc(b) << std::endl;
	std::cout << "... score: " << bestest << std::endl;
	_result = b;

}

void Aligner::align()
{
	if (Control::valueForKey("align-rotation") == "true")
	{
		rotation();
		mat3x3 r = result();
		_ori1->rotateRoundCentre(r, empty_vec3(), VagFFTPtr(), 1, true);
		emit focusOnCentre();
		altered()->drawSlice(-1, "drawreal_" + i_to_str(number()) + "_rot");
	}

	if (Control::valueForKey("align-translation") == "true")
	{
		translation();
		vec3 origin = _ori1->origin();
		vec3_add_to_vec3(&origin, _translation);
		_ori1->setOrigin(origin);
	}

	double cor = -_ori1->rotateRoundCentre(make_mat3x3(), empty_vec3(), _ori0); 
	std::cout << "Rough correlation: " << cor << std::endl;
	altered()->writeMRC("rough.mrc");
	
	if (Control::valueForKey("rigid-body") == "true")
	{
		microAdjustments();
		cor = -_ori1->rotateRoundCentre(make_mat3x3(), empty_vec3(), _ori0); 
		altered()->drawSlice(-1, "drawreal_" + i_to_str(number()) + "_zadjust");
		std::cout << "Adjusted correlation: " << cor << std::endl;
	}
	
	if (Control::valueForKey("distortion") == "true")
	{
		std::cout << "Keypoints: " << cor << std::endl;
		altered()->refineKeypoints(_ori0);
		altered()->drawSlice(-1, "drawreal_" + i_to_str(number()) + "_zkeypoints");
		cor = -_ori1->rotateRoundCentre(make_mat3x3(), empty_vec3(), _ori0); 
		std::cout << "Keypoint correlation: " << cor << std::endl;
		altered()->writeMRC("keypoint.mrc");
	}

	emit resultReady();
}

void Aligner::finishRefinement()
{
	vec3 trans = _microTrans;
	mat3x3 rot = mat3x3_rotate(_microAngles.x, _microAngles.y, _microAngles.z);

	double correl = _ori1->rotateRoundCentre(rot, trans, _ori0, _scale, true);
	correl = _ori1->rotateRoundCentre(make_mat3x3(), empty_vec3(), _ori0);
	std::cout << "Interim correlation: " << correl << std::endl;
	_microAngles = empty_vec3();
	_microTrans = empty_vec3();
	_scale = 1;
}

void Aligner::microAdjustments()
{
	NelderMeadPtr neld = NelderMeadPtr(new RefinementNelderMead());
	neld->setVerbose(true);
	neld->setCycles(100);
	neld->setEvaluationFunction(Aligner::score, this);
	neld->setJobName("rigid_body");

	AnyPtr ax = AnyPtr(new Any(&_microTrans.x));
	neld->addParameter(&*ax, Any::get, Any::set, 2, 0.1);
	AnyPtr ay = AnyPtr(new Any(&_microTrans.y));
	neld->addParameter(&*ay, Any::get, Any::set, 2, 0.1);
	AnyPtr az = AnyPtr(new Any(&_microTrans.z));
	neld->addParameter(&*az, Any::get, Any::set, 2, 0.1);

	AnyPtr rx = AnyPtr(new Any(&_microAngles.x));
	neld->addParameter(&*rx, Any::get, Any::set, deg2rad(3), deg2rad(0.01));
	AnyPtr ry = AnyPtr(new Any(&_microAngles.y));
	neld->addParameter(&*ry, Any::get, Any::set, deg2rad(3), deg2rad(0.01));
	AnyPtr rz = AnyPtr(new Any(&_microAngles.z));
	neld->addParameter(&*rz, Any::get, Any::set, deg2rad(3), deg2rad(0.01));
	AnyPtr s = AnyPtr(new Any(&_scale));
	neld->addParameter(&*s, Any::get, Any::set, 0.05, 0.001);

	std::cout << "Starting microadjustments..." << std::endl;
	neld->refine();
	finishRefinement();
}

double Aligner::adjustScore()
{
	vec3 trans = _microTrans;
	mat3x3 rot = mat3x3_rotate(_microAngles.x, _microAngles.y, _microAngles.z);

	double cor = -_ori1->rotateRoundCentre(rot, trans, _ori0, _scale); 
	return cor;
}
