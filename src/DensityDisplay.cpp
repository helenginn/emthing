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
#include <vaggui/Density2GL.h>
#include <h3dsrc/Icosahedron.h>
#include <QMenu>
#include <iostream>

DensityDisplay::DensityDisplay(QWidget *parent) : SlipGL(parent)
{
	_activeTrace = NULL;
	_ico = new Icosahedron();
	setBackground(1, 1, 1, 1);
	_ico->setColour(0.5, 0.5, 0.5);
	_ico->setPosition(make_vec3(260, 260, 260));
	addObject(_ico, true);
	setZFar(1000);
	_worker = NULL;
}

void DensityDisplay::addDensity(VagFFTPtr fft)
{
	_fft = fft;
	_density = new Density2GL();
	_density->setDims(25, 25, 25);
	_density->setResolution(3.0);

	for (size_t i = 0; i < 40; i++)
	{
		_density->nudgeDensity(1);
	}

	_density->makeNewDensity(fft);

	std::cout << "Made density" << std::endl;
	addObject(_density);
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
	QMenu *menu = new QMenu;

	menu->addAction("Start trace", this, &DensityDisplay::startTrace);

	menu->exec(p);
}

void DensityDisplay::done()
{
	disconnect(this, SIGNAL(begin()), nullptr, nullptr);
	disconnect(_activeTrace, SIGNAL(resultReady()), nullptr, nullptr);

	_worker->quit();
}

void DensityDisplay::startTrace()
{
	if (!_fft)
	{
		return;
	}
	
	double threshold = 4.0 * _density->getSigma();
	threshold += _density->getMean();

	Trace *t = new Trace();
	t->setFFT(_fft);
	t->setMinimum(threshold);
	vec3 start = _ico->centroid();
	t->setStart(start);
	_objects.insert(_objects.begin(), t);
	prepareWorkForObject(t);
	_activeTrace = t;

	connect(this, SIGNAL(begin()), t, SLOT(trace()));
	connect(t, SIGNAL(resultReady()), this, SLOT(done()));
	_worker->start();

	emit begin();
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
