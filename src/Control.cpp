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
#include <iostream>

Control::Control() : Dictator::Dictator()
{
	_properties["align-rotation"] = "true";
	_properties["align-translation"] = "true";
	_properties["crop-fraction"] = "true";
	_properties["make-plans"] = "true";
	_properties["write-distortion"] = "false";

}

void Control::finished()
{
	if (_properties["gui"] != "true")
	{
		exit(0);
	}
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
	else if (first == "invert-hand")
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
	else if (first == "write-distortion")
	{
		_properties[first] = last;
	}
	else if (first == "gui")
	{
		_view->show();
		_properties["gui"] = "true";
	}
	else if (first == "load-map")
	{
		_view->addMRCin(last);
	}
	else if (first == "write-last")
	{
		_display->writeLast(last);
	}
	else if (first == "correlate-last")
	{
		_display->correlateLast();
	}
	else if (first == "mask-with-pdb")
	{
		_display->maskWithPDB(last);
	}
	else if (first == "drop-data")
	{
		_display->dropData();
	}
	else if (first == "load-from-list")
	{
		_display->loadFromFileList(last);
	}
	else if (first == "correlate-all")
	{
		_display->correlateAll(last);
	}
	else if (first == "take-picture")
	{
		_display->pictureLast(last);
	}
	else if (first == "align-last")
	{
		_display->alignLast();
		return false;
	}

	return true;
}

void Control::help()
{
	std::cout << "********************************" << std::endl;
	std::cout << "EM thing by Helen (name pending)" << std::endl;
	std::cout << "********************************" << std::endl;
	std::cout << std::endl;
	std::cout << "Usage: em_thing <commands>" << std::endl;
	std::cout << "Commands are sequentially executed." << std::endl;
	std::cout << std::endl;
	std::cout << "Command type: order" << std::endl;

	std::cout << "\tquit\t\t\t\t";
	std::cout << "End execution of program." << std::endl;

	std::cout << "\tgui\t\t\t\t";
	std::cout << "Launch GUI of program." << std::endl;

	std::cout << "\tload-map=<filename>\t\t";
	std::cout << "Load map in MRC format. First loaded MRC becomes "
	"the reference map for future calculations." << std::endl;

	std::cout << "\talign-last=<filename>\t\t";
	std::cout << "Aligns the last loaded map to reference (see toggles)."
	<< std::endl;

	std::cout << "\twrite-last=<filename>\t\t";
	std::cout << "Write out last map, including edits, to filename."
	<< std::endl;
	std::cout << std::endl;

	std::cout << "\ttake-picture=<filename>\t\t";
	std::cout << "Saves PNG file of last map projected down Z axis."
	<< std::endl;
	std::cout << std::endl;

	std::cout << "Command type: values" << std::endl;

	std::cout << "\tcrop-fraction=<floating point>\t";
	std::cout << "Determines the size of the box to crop incoming maps. ";
	std::cout << "Numbers outside the range (0, 1) will result in ";
	std::cout << "\n\t\t\t\t\t";
	std::cout << "automatic determination according to signal levels. ";
	std::cout << "Default is to automatically determine."
	<< std::endl;


	std::cout << std::endl;
	std::cout << "Command type: toggle" << std::endl;

	std::cout << "\tinvert-hand=true|false\t\t";
	std::cout << "Inverts hand in the process of aligning maps." <<
	"(default: false)" << std::endl;

	std::cout << "\talign-translation=true|false\t";
	std::cout << "Calculate translational shift during alignment (n.b. not "
	"presently written to output map). (default: true)" << std::endl;

	std::cout << "\talign-rotation=true|false\t";
	std::cout << "Calculate rotational shift during alignment. "
	"(default: true)" << std::endl;

	std::cout << "\trigid-body=true|false\t\t";
	std::cout << "Execute rigid body alignment after coarse alignment. "
	"(default: false)" << std::endl;

	std::cout << std::endl;
	exit(0);
}
