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

#include "Control.h"
#include "DensityDisplay.h"
#include "VolumeView.h"

Control::Control() : Dictator::Dictator()
{
	_properties["align-rotation"] = "true";
	_properties["align-translation"] = "true";
	_properties["crop-fraction"] = "true";

}

bool Control::processRequest(std::string first, std::string last)
{
	if (first == "quit")
	{
		exit(0);
	}
	else if (first == "rigid-body" || first == "distortion" ||
	         first == "align-translation" || first == "align-rotation")
	{
		_properties[first] = last;
	}
	else if (first == "crop-fraction")
	{
		_properties[first] = last;
	}
	else if (first == "distortion")
	{
		_properties[first] = last;
	}
	else if (first == "gui")
	{
		_view->show();
	}
	else if (first == "load-map")
	{
		_view->addMRCin(last);
	}
	else if (first == "align-last")
	{
		_display->alignLast();
		return false;
	}

	return true;
}
