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

#include "DensityDisplay.h"
#include "Trace.h"
#include "Aligner.h"
#include <vaggui/Density2GL.h>
#include <libsrc/FFT.h>
#include "DistortMap.h"
#include "Control.h"
#include <h3dsrc/Icosahedron.h>
#include <hcsrc/FileReader.h>
#include <hcsrc/maths.h>
#include <QMenu>
#include <iostream>

DensityDisplay::DensityDisplay(QWidget *parent) : SlipGL(parent)
{
	_activeTrace = NULL;
	_align = NULL;
	_ico = new Icosahedron();
	setBackground(1, 1, 1.0, 1);
	_ico->setColour(0.2, 0.2, 0.2);
	_ico->setPosition(make_vec3(0, 0, 0));
	addObject(_ico, true);
	setZFar(1000);
	_worker = NULL;
}

void DensityDisplay::addDensity(VagFFTPtr vagfft)
{
	DistortMapPtr fft = DistortMapPtr(new DistortMap(*vagfft));
	VagFFTPtr toVag = boost::static_pointer_cast<VagFFT>(fft);
	_fft = fft;
	_density = new Density2GL();
	_density->setDims(30, 30, 30);
	_density->setResolution(1.0);

	for (size_t i = 0; i < 40; i++)
	{
		_density->nudgeDensity(1);
	}
	
	float h = (360 * 1.605 * _densities.size());
	while (h > 360) h -= 360;
	float s = 80;
	float v = 100;
	hsv_to_rgb(h, s, v);
	_fft->setColour(h, s, v);

	_density->setDefaultColour(h, s, v);
	_density->makeNewDensity(toVag);
	addObject(&*fft);

	fft->drawSlice(-1, "drawreal_" + i_to_str(_ffts.size()));

	addObject(_density);
	_densities.push_back(_density);
	_ffts.push_back(fft);
	
	if (_ffts.size() == 1)
	{
		centre();
	}
	else
	{
		mat3x3 toReal = fft->toReal();
		std::cout << mat3x3_desc(toReal) << std::endl;
		mat3x3 target = _ffts[0]->toReal();
		
		vec3 targo = _ffts[0]->origin();
		vec3 centre_of_target = make_vec3(target.vals[0] / 2,
		                                  target.vals[4] / 2,
		                                  target.vals[8] / 2);
		vec3_add_to_vec3(&centre_of_target, targo);
		
		vec3 myo = fft->origin();
		vec3 my_centre = make_vec3(toReal.vals[0] / 2,
		                           toReal.vals[4] / 2,
		                           toReal.vals[8] / 2);
		vec3_add_to_vec3(&my_centre, myo);

		vec3 oriplus = vec3_subtract_vec3(centre_of_target, my_centre);

		vec3_add_to_vec3(&myo, oriplus);
		_fft->setOrigin(myo);
	}

}

void DensityDisplay::correctIco()
{
	vec3 c = getCentre();
	mat4x4 inv = mat4x4_inverse(_model);
	c = mat4x4_mult_vec(inv, c);
	_ico->setPosition(c);
}

void DensityDisplay::mouseMoveEvent(QMouseEvent *e)
{
	SlipGL::mouseMoveEvent(e);

	correctIco();
}

void DensityDisplay::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() == Qt::RightButton && !_moving)
	{
		rightClickMenu(e->globalPos());
	}

	SlipGL::mouseReleaseEvent(e);
	correctIco();
}

void DensityDisplay::rightClickMenu(QPoint p)
{
	return;
	QMenu *menu = new QMenu;

	menu->exec(p);
}

void DensityDisplay::done()
{
	disconnect(this, SIGNAL(begin()), nullptr, nullptr);
	disconnect(_align, SIGNAL(resultReady()), nullptr, nullptr);
	
	_worker->quit();
	
	if (_align != NULL)
	{
		_align->altered()->drawSlice(-1, "drawreal_" +
		i_to_str(_align->number()) + "_after");

		removeObject(_align);

		if (_alignables.size() > 0)
		{
			_alignables.erase(_alignables.begin());
		}

		delete _align;
		_align = NULL;
		startNextAlignment();
	}
}

void DensityDisplay::startNextAlignment()
{
	if (_alignables.size() == 0)
	{
		_dictator->incrementJob();
		return;
	}

	_align = _alignables[0];
	_objects.insert(_objects.begin(), _align);
	prepareWorkForObject(_align);
	
	connect(this, SIGNAL(begin()), _align, SLOT(align()));
	connect(_align, SIGNAL(resultReady()), this, SLOT(done()));
	connect(_align, SIGNAL(focusOnCentre()), this, SLOT(centre()));
	_worker->start();

	emit begin();
}

void DensityDisplay::alignLast()
{
	if (_ffts.size() <= 1)
	{
		std::cout << "Only one FFT." << std::endl;
		return;
	}
	
	focusOnPosition(empty_vec3(), 100);
	
	_align = new Aligner(_ffts[0], _ffts.back());
	_align->setFFTNumber(_ffts.size() - 1);
	_alignables.push_back(_align);
	
	startNextAlignment();

}

void DensityDisplay::alignDensities()
{
	if (_ffts.size() <= 1)
	{
		return;
	}
	
	focusOnPosition(empty_vec3(), 100);
	
	for (size_t i = 1; i < _ffts.size(); i++)
	{
		_align = new Aligner(_ffts[0], _ffts[i]);
		_align->setFFTNumber(i);
		_alignables.push_back(_align);
	}
	
	startNextAlignment();
}

void DensityDisplay::centre()
{
	vec3 frac = _ffts[0]->fractionalCentroid();
	mat3x3 toReal = _ffts[0]->toReal();
	mat3x3_mult_vec(toReal, &frac);
	vec3 ori = _ffts[0]->origin();
	vec3_add_to_vec3(&frac, ori);
	focusOnPosition(frac, 20);
}

bool DensityDisplay::prepareWorkForObject(QObject *object)
{
	if (object == NULL)
	{
		return false;
	}

	if (_worker && _worker->isRunning())
	{
		std::cout << "Waiting for worker to finish old job" << std::endl;
		_worker->wait();
	}
	
	if (!_worker)
	{
		_worker = new QThread();
	}

	object->moveToThread(_worker);

	return true;
}
