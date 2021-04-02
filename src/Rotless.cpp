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

#include "Rotless.h"
#include "MRCImage.h"
#include <hcsrc/FileReader.h>
#include <QTimer>
#include <QApplication>
#include <QDesktopWidget>
#include <h3dsrc/Quad.h>

Rotless::Rotless(QWidget *parent, MRCImage *im) : SlipGL(parent)
{
	_image = im;
	_imageTexture = 0;
	_timer->stop();
	_timer->setSingleShot(true);
	_oneD = false;
}

size_t Rotless::textureHeight()
{
	return _oneD ? 1 : 128;
}

size_t Rotless::textureWidth()
{
	return 128;
}

void Rotless::useShaders(std::string v, std::string f)
{
	_vsh = get_file_contents(v);
	_fsh = get_file_contents(f);
	
	std::cout << "Using " << v << " " << f << std::endl;
}

void Rotless::initializeGL()
{
	SlipGL::initializeGL();

	prepareRenderToTexture(1);
	preparePingPongBuffers(textureWidth(), textureHeight());
	
	Quad *q = quad();
	q->changeProgram(_vsh, _fsh);
	q->prepareTextures(this);
	quad()->setThreshold(0.0);
	quad()->setHorizontal(false);
}

void Rotless::changeImage(MRCImage *newImage)
{
	_image = newImage;
	glDeleteTextures(1, &_imageTexture);
	bindTexture();
	update();
}

void Rotless::bindTexture()
{
	glGenTextures(1, &_imageTexture);
	glBindTexture(GL_TEXTURE_2D, _imageTexture);
	
	QImage conv = _image->convertToFormat(QImage::Format_RGBA8888);

	GLint intform = GL_RGBA8;
	GLenum myform = GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, intform, conv.width(), conv.height(),
	             0, myform, GL_UNSIGNED_BYTE, conv.constBits());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	quad()->setTexture(1, _imageTexture);
}

void Rotless::tidyUpFinal()
{
	size_t size = textureWidth() * textureHeight() * 4;
	float *data = new float[size];
	glReadPixels(0, 0, textureWidth(), textureHeight(), 
	             GL_RGBA, GL_FLOAT, data);

	float sum = 0;
	float sumsq = 0;
	float count = 0;
	for (size_t i = 0; i < size; i += 4)
	{
		float val = data[i];
		
		if (val < 1e-6)
		{
			continue;
		}

		sum += val;
		sumsq += val * val;
		count++;
	}
	
	float stdev = sqrt(sumsq / count - sum * sum / (count * count));
	if (stdev != stdev)
	{
		stdev = 1;
	}
	
	float mean = sum / count;
	
	_quad->setHorizontal(true);
	quad()->setThreshold(stdev);
	quad()->setOther(mean);
	quad()->setTexture(1, _pingPongMap[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0); 
	
	delete [] data;

}

void Rotless::addBytes(int which)
{
	if (_oneD)
	{
		return;
	}

	tidyUpFinal();
	quad()->setTexture(1, _pingPongMap[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, _pingPongFbo[1]); 
	renderQuad();
	
	size_t size = textureWidth() * textureHeight() * 4;
	float *data = new float[size];
	unsigned char *chdata = new unsigned char[size / 4];

	glReadPixels(0, 0, textureWidth(), textureHeight(), 
	             GL_RGBA, GL_FLOAT, data);
	
	for (size_t i = 0; i < size; i += 4)
	{
		float val = data[i];
		if (val > 1) val = 1;
		if (val < 0) val = 0;
		unsigned char ch = val * 255;
		chdata[i/4] = ch;
	}

	QImage print;
	print = QImage(chdata, textureWidth(), textureHeight(),
	               QImage::Format_Grayscale8).copy();
	_image->setFingerprint(&print);
}

void Rotless::specialQuadRender()
{
	float threshold = 180.0;
	
	if (_imageTexture == 0)
	{
		_quad->setHorizontal(true);
		quad()->setTexture(1, _pingPongMap[0]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0); 
		return;
	}

	_image->clearFingerprints();

	quad()->setTexture(1, _imageTexture);
	_quad->setHorizontal(false);
	_quad->setThreshold(threshold);
	glBindFramebuffer(GL_FRAMEBUFFER, _pingPongFbo[0]); 
	renderQuad();
	addBytes(0);
	
	while (!_oneD && threshold < 134)
	{
		threshold += 45;
		_quad->setHorizontal(false);
		quad()->setThreshold(threshold);
		quad()->setTexture(1, _imageTexture);
		glBindFramebuffer(GL_FRAMEBUFFER, _pingPongFbo[0]); 
		renderQuad();
		addBytes(0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0); 
	renderQuad();
}
