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

#ifndef __em__Rotless__
#define __em__Rotless__

#include <h3dsrc/SlipGL.h>

class MRCImage;

class Rotless : public SlipGL
{
public:
	Rotless(QWidget *parent, MRCImage *image);

	void useShaders(std::string v, std::string f);
	
	void set1D(bool d)
	{
		_oneD = d;
	}

	size_t textureHeight();
	size_t textureWidth();

	void changeImage(MRCImage *newImage);
protected:
	virtual void specialQuadRender();
	virtual void initializeGL();
	void bindTexture();
	void tidyUpFinal();
	void addBytes(int which);

private:
	bool _oneD;
	MRCImage *_image;
	GLuint _imageTexture;
	std::string _vsh;
	std::string _fsh;
};

#endif
