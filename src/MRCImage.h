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

#ifndef __em__MRCImage__
#define __em__MRCImage__

#include <QImage>
#include <QTreeWidgetItem>
#include <map>

class MRCImage : public QImage
{
public:
	MRCImage(int width, int height);
	
	void setName(std::string name)
	{
		_name = name;
	}
	
	std::string name()
	{
		return _name;
	}

	void setRawData(float *ptr);
	void redraw();
	
	void clearFingerprints();
	
	void setFingerprint(QImage *q);
	
	bool hasFingerprint()
	{
		return (_fingerprints.size() > 0);
	}
	
	void allocateDifferprint(int count);
	void populateDifferprint(int i, MRCImage *other);
	
	void setThresholds(float min, float max)
	{
		_min = min;
		_max = max;
	}

	double correlWith(MRCImage *other);
	double diffCorrelWith(MRCImage *other);
	void estimateRange();
	void writeImage();
	void blur();

	void bin(size_t factor);
	void reverse();
private:
	size_t fingerprintCount()
	{
		return _fingerprints.size();
	}
	
	QImage *fingerprint(int i)
	{
		return _fingerprints[i];
	}

	std::vector<float> _values;
	std::string _name;

	float _min;
	float _max;
	size_t _width;
	size_t _height;
	std::vector<QImage *> _fingerprints;

	signed char *differprint(int i);
	signed char *differprint(MRCImage *other);
	
	std::map<QImage *, signed char *> _diffPtrs;
	
	std::vector<signed char> _differprint;
};

#endif
