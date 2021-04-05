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

#ifndef __em__Control__
#define __em__Control__

#include <h3dsrc/Dictator.h>

class DensityDisplay;
class VolumeView;

class Control : public Dictator
{
public:
	Control();

	void setView(VolumeView *view)
	{
		_view = view;
	}

	void setDisplay(DensityDisplay *display)
	{
		_display = display;
	}

protected:
	virtual bool processRequest(std::string first, std::string last);
private:
	VolumeView *_view;
	DensityDisplay *_display;

};

#endif
