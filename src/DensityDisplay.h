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

#ifndef __em__DensityDisplay__
#define __em__DensityDisplay__

#include <h3dsrc/SlipGL.h>
#include <libsrc/shared_ptrs.h>
#include <QThread>

class Density2GL;
class Dictator;
class Icosahedron;
class Trace;
class Aligner;
class DistortMap; typedef boost::shared_ptr<DistortMap> DistortMapPtr;

class DensityDisplay : public SlipGL
{
Q_OBJECT
public:
	DensityDisplay(QWidget *parent);

	void addDensity(DistortMapPtr fft);
	
	void setDictator(Dictator *dict)
	{
		_dictator = dict;
	}
	
	void setSymFile(std::string str)
	{
		_symFile = str;
	}
	
	void findReferenceBySymmetry();
	void alignLastTo(std::string name);
	void writeLast(std::string filename);
	void pictureLast(std::string filename);
	void correlateLast();
	void correlateAll(std::string filename);
	void loadFromFileList(std::string filename);
	void maskWithPDB(std::string pdb);
	void dropData();
	void setAsReference(std::string name);
public slots:
	void centre();
	void done();
signals:
	void begin();
protected:
	void correctIco();
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
private:
	void startNextAlignment();
	bool prepareWorkForObject(QObject *object);
	void rightClickMenu(QPoint p);
	std::vector<Density2GL *> _densities;
	std::vector<DistortMapPtr> _ffts;
	std::vector<DistortMapPtr> _refs;
	std::map<std::string, DistortMapPtr> _name2Ref;
	std::map<DistortMapPtr, std::string> _ref2Name;
	std::map<DistortMapPtr, DistortMapPtr> _fft2Ref;
	std::map<std::string, double> _scores;
	std::vector<Aligner *> _alignables;
	
	std::string _symFile;
	Density2GL * _density;
	Icosahedron *_ico;
	QThread *_worker;
	Trace *_activeTrace;
	Aligner *_align;
	Dictator *_dictator;
	
	bool _allRefs;

};

#endif
