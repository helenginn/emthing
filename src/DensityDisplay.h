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
class Icosahedron;
class Trace;


class DensityDisplay : public SlipGL
{
Q_OBJECT
public:
	DensityDisplay(QWidget *parent);

	void addDensity(VagFFTPtr fft);
public slots:
	void startTrace();
	void done();
signals:
	void begin();
protected:
	void correctIco();
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
private:
	bool prepareWorkForObject(QObject *object);
	void rightClickMenu(QPoint p);
	Density2GL *_density;
	Icosahedron *_ico;
	VagFFTPtr _fft;
	QThread *_worker;
	Trace *_activeTrace;

};

#endif
