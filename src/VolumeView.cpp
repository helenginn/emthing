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

#include "VolumeView.h"
#include "MRCin.h"
#include "DensityDisplay.h"
#include <libsrc/FFT.h>
#include "Control.h"

class DistortMap; typedef boost::shared_ptr<DistortMap> DistortMapPtr;

VolumeView::VolumeView(QWidget *parent) : QMainWindow(parent)
{
	_display = new DensityDisplay(this);
	setGeometry(200, 200, 800, 800);
	_display->setGeometry(0, 0, 800, 800);
	_display->setFocusPolicy(Qt::ClickFocus);
	_display->show();

}

void VolumeView::addCommand(std::string command)
{
	_args.push_back(command);
}

void VolumeView::addMRCin(std::string mrc)
{
	MRCin *in = new MRCin(mrc, true);
	DistortMapPtr fft = in->getVolume();
	_display->addDensity(fft);
}

void VolumeView::start()
{
	Control *c = new Control();
	c->setView(this);
	c->setDisplay(_display);
	c->setArgs(_args);
	_display->setDictator(c);
	c->run();
}
