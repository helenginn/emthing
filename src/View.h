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

#ifndef __em__View__
#define __em__View__

#include <QMainWindow>

class MRCin;
class Rotless;
class QTreeWidget;
class QTreeWidgetItem;

class View : public QMainWindow
{
Q_OBJECT
public:
	View(QWidget *parent);

public slots:
//	void sliderMoved(int val);
	void accessRandomItem(QTreeWidgetItem *current);
	void fingerAll();
	void diffAll();

	void contextMenu(const QPoint &p);
	void addMRCin(std::string filename);
	
	Rotless *finger()
	{
		return _finger;
	}

	Rotless *twizzle()
	{
		return _rotless;
	}
private:
	bool getMRC();

	void csv(std::string filename);
	QTreeWidget *_tree;
	std::vector<MRCin *> _ins;
	Rotless *_finger;
	Rotless *_rotless;
	bool _fingered;

};

#endif
