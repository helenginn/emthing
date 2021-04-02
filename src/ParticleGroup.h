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

#ifndef __em__ParticleGroup__
#define __em__ParticleGroup__

#include <QTreeWidgetItem>
#include <c4xsrc/Screen.h>

class View;
class MRCin;
class MRCImage;
class Screen;

class ParticleGroup : public QTreeWidgetItem, public C4XAcceptor
{
public:
	ParticleGroup(QTreeWidget *parent);
	ParticleGroup(ParticleGroup *parent);
	
	void setView(View *view)
	{
		_view = view;
	}
	void fingerprint();

	void addImage(MRCImage *image);

	void loadFromMRCin(MRCin *mrcin);
	void makeDifferences();
	void csv(std::string filename, bool diff = false);

	size_t imageCount();
	MRCImage *image(int i);
	std::vector<MRCImage *> images();

	void prepareCluster4x(bool diffs);
	
	virtual void finished();
private:
	Screen *_screen;
	std::vector<MRCImage *> _images;
	
	std::map<std::string, MRCImage *> _nameMap;

	QTreeWidget *_tree;
	View *_view;
};

#endif
