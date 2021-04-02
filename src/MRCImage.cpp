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

#include "MRCImage.h"
#include <iostream>
#include <sstream>
#include <hcsrc/maths.h>
#include <hcsrc/FileReader.h>

MRCImage::MRCImage(int width, int height)
: QImage(width, height, QImage::Format_Grayscale8)
{
	_width = width;
	_height = height;
	_min = -100;
	_max = 100;

}

void MRCImage::setRawData(float *ptr)
{
	_values.resize(_width * _height);
	memcpy(&_values[0], ptr, sizeof(float) * _width * _height);
}

void MRCImage::blur()
{
	const unsigned char *bits = constBits();

	unsigned char *cooked;
	cooked = new unsigned char[width() * height()];

	for (size_t y = 1; y < height() - 1; y++)
	{
		for (size_t x = 1; x < width() - 1; x++)
		{
			int sum = 0;

			for (int j = -1; j <= +1; j++)
			{
				for (int i = -1; i <= +1; i++)
				{
					int idx = (y + j) * width() + x + i;
					unsigned char val = bits[idx]; 
					sum += val;
				}
			}
		
			sum /= 9;

			int idx = (y * width()) + x;

			cooked[idx] = sum;
		}
	}
	
	for (size_t y = 1; y < height(); y++)
	{
		int idx = y * width();
		cooked[idx] = 0;
		cooked[idx - 1] = 0;
	}
	
	for (size_t x = 0; x < width(); x++)
	{
		int idx = (height() - 1) * width() + x;
		cooked[x] = 0;
		cooked[idx] = 0;
	}

	QImage newImage = QImage(reinterpret_cast<unsigned char *>(cooked), 
	                         width(), height(), QImage::Format_Grayscale8);
	QImage copy = newImage.copy();
	this->swap(copy);

	delete [] cooked;
}

void MRCImage::estimateRange()
{
	CorrelData cd = empty_CD();
	int cx = _width / 2;
	int cy = _height / 2;
	int rad = 14;
	for (int y = cy - rad; y <= cy + rad; y++)
	{
		for (int x = cx - rad; x <= cx + rad; x++)
		{
			float val = _values[y * _width + x];
			add_to_CD(&cd, val, val);
		}
	}
	
	double xm, ym, xs, ys;
	means_stdevs_CD(cd, &xm, &ym, &xs, &ys);

	_min = xm - xs * 4;
	_max = xm + xs * 4;
}

void MRCImage::bin(size_t factor)
{
	const unsigned char *bits = constBits();

	unsigned char *cooked;
	cooked = new unsigned char[width() * height() / factor / factor];
	int count = 0;
	
	CorrelData cd = empty_CD();
	
	for (size_t y = 0; y < height() * width(); y += width() * factor)
	{
		for (size_t x = 0; x < width(); x += factor)
		{
			unsigned char lowest = 255;
			unsigned char highest = 0;
			int sum = 0;

			for (size_t j = 0; j < factor; j++)
			{
				for (size_t i = 0; i < factor; i++)
				{
					int idx = y + (width() * j) + x + i;
					unsigned char val = bits[idx]; 
					sum += val;
					if (val < lowest) lowest = val;
					if (val > highest) highest = val;
				}
			}
			
			sum /= factor * factor;
			
			int chosen = (255 - highest < lowest ? highest : lowest);
			
			add_to_CD(&cd, sum, sum);

			cooked[count] = sum;
			count++;
		}
	}

	double xm, ym, xs, ys;
	means_stdevs_CD(cd, &xm, &ym, &xs, &ys);
	
	for (size_t i = 0; i < count; i++)
	{
		float val = cooked[i];
		val -= xm;
		val /= xs;
		val *= 32;
		val += 128;
		if (val > 255) val = 255;
		if (val < 0) val = 0;
		cooked[i] = val;
	}

	QImage newImage = QImage(reinterpret_cast<unsigned char *>(cooked), 
	                         width() / factor, height() / factor, 
	                         QImage::Format_Grayscale8);
	QImage copy = newImage.copy();
	this->swap(copy);

	delete [] cooked;
}

void MRCImage::reverse()
{
	unsigned char *cooked = new unsigned char[width() * height()];
	for (size_t i = 0; i < width() * height(); i++)
	{
		cooked[i] = 255 - constBits()[i];
	}

	QImage newImage = QImage(reinterpret_cast<unsigned char *>(cooked), 
	                         width(), height(), 
	                         QImage::Format_Grayscale8);
	QImage copy = newImage.copy();
	this->swap(copy);

	delete [] cooked;
}

void MRCImage::redraw()
{
	unsigned char *cooked = new unsigned char[_width * _height];

	for (size_t y = 0; y < _height * _width; y += _width)
	{
		for (size_t x = 0; x < _width; x++)
		{
			float val = _values[y + x];
			val -= _min;
			val /= (_max - _min);
			val *= val;
			unsigned char conv = val * 256;
			cooked[y + x] = conv;
		}
	}
	
	QImage newImage = QImage(reinterpret_cast<unsigned char *>(cooked), 
	                         _width, _height, QImage::Format_Grayscale8);
	QImage copy = newImage.copy();//.scaled(128, 128);
	this->swap(copy);

	delete [] cooked;
}

void MRCImage::clearFingerprints()
{
	for (size_t i = 0; i < _fingerprints.size(); i++)
	{
		delete _fingerprints[i];
	}
	
	_fingerprints.clear();
}

void MRCImage::setFingerprint(QImage *q)
{
	QImage scaled = q->copy().scaled(width(), height());
	int prop = (width() / 2);
	_fingerprints.push_back(new QImage(scaled.copy(0, prop, prop , prop)));
}

double MRCImage::diffCorrelWith(MRCImage *other)
{
	CorrelData cd = empty_CD();

	signed char *aptr = differprint(0);
	signed char *bptr = other->differprint(0);

	for (size_t y = 0; y < _differprint.size(); y++)
	{
		signed char apix = aptr[y];
		signed char bpix = bptr[y];

		float a = (apix+0) / (float)255;
		float b = (bpix+0) / (float)255;

		if (apix == 0 && bpix == 0)
		{
			continue;
		}

		add_to_CD(&cd, (a), (b));
	}

	return evaluate_CD(cd);
}

double MRCImage::correlWith(MRCImage *other)
{
	if (fingerprintCount() != other->fingerprintCount() ||
	    fingerprintCount() == 0)
	{
		return NAN;
	}

	CorrelData cd = empty_CD();
	
	size_t w = fingerprint(0)->width();
	size_t h = fingerprint(0)->height();
	
	std::ostringstream str;
	double count = 0;
	
	for (size_t n = 0; n < fingerprintCount(); n++)
	{
	for (size_t y = 0; y < h; y++)
	{
		for (size_t x = 0; x < w; x++)
		{
			if (y / float(h) < 0.05 || x / float(w) < 0.05)
			{
				continue;
			}

			if (x < y)
			{
				continue;
			}
			
			if (sqrt(y * y + x * x) > w)
			{
				continue;
			}

			int idx = y * w + x;

			unsigned char apix = fingerprint(n)->constBits()[idx];
			unsigned char bpix = other->fingerprint(n)->constBits()[idx];

			if (apix == 0 && bpix == 0)
			{
//				continue;
			}

			float a = (apix+0) / (float)255;
			float b = (bpix+0) / (float)255;

			add_to_CD(&cd, (a), (b));
		}
	}
	}
	
	double correl = evaluate_CD(cd);
	
	return correl;
}

signed char *MRCImage::differprint(MRCImage *other)
{
	if (_diffPtrs.count(other) == 0)
	{
		return NULL;
	}
	
	return _diffPtrs[other];
}

signed char *MRCImage::differprint(int i)
{
	size_t size = fingerprint(0)->width() * fingerprint(0)->height();
	return &_differprint[i * size];
}

void MRCImage::allocateDifferprint(int count)
{
	size_t size = fingerprint(0)->width() * fingerprint(0)->height();
	_differprint.clear();
	_differprint.resize(count * size);
}

void MRCImage::populateDifferprint(int i, MRCImage *other)
{
	signed char *ptr = differprint(i);

	size_t w = fingerprint(0)->width();
	size_t h = fingerprint(0)->height();

	for (size_t y = 0; y < h; y++)
	{
		for (size_t x = 0; x < w; x++)
		{
			int idx = y * w + x;

			unsigned char apix = fingerprint(0)->constBits()[idx];
			unsigned char bpix = other->fingerprint(0)->constBits()[idx];
			
			ptr[idx] = (char)apix - (char)bpix;
		}
	}
	
	_diffPtrs[other] = ptr;
}

void MRCImage::writeImage()
{
	for (size_t i = 0; i < fingerprintCount(); i++)
	{
		QImage *im = fingerprint(i);
		std::string fn = name() + "_" + i_to_str(i) + ".png";
		im->save(QString::fromStdString(fn));
	}

	/*
	std::vector<unsigned char> diffs;
	diffs.resize(_differprint.size());
	
	for (size_t i = 0; i < diffs.size(); i++)
	{
		diffs[i] = _differprint[i] + 256/2;
	}
	
	size_t size = fingerprint(0)->width() * fingerprint(0)->height();
	
	int num = diffs.size() / size;

	QImage write = QImage(&diffs[0], fingerprint(0)->width(),
	                      num * fingerprint(0)->height(),
	                      QImage::Format_Grayscale8);
	std::string fn = name() + "_diff.png";
	std::cout << fn << std::endl;
	write.save(QString::fromStdString(fn));
	*/
}
