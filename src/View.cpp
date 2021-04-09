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

#include "View.h"
#include "MRCin.h"
#include "Rotless.h"
#include "MRCImage.h"
#include "ParticleGroup.h"
#include "Particle.h"

#include <h3dsrc/Dialogue.h>

#include <iostream>
#include <fstream>

#include <QMenu>
#include <QLabel>
#include <QSlider>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTreeWidget>

View::View(QWidget *parent) : QMainWindow(parent)
{
	_fingered = false;
	/*
	std::string fn = openDialogue(this, "Find MRC file", "*.mrc",
	                              false, false);
	*/
	
	QWidget *window = new QWidget();
	QHBoxLayout *bbox = new QHBoxLayout();
	window->setLayout(bbox);
	ParticleGroup *group = NULL;
	
	{
		_tree = new QTreeWidget(this);
		_tree->setMinimumSize(300, 512);
		
		group = new ParticleGroup(_tree);
		group->setView(this);
		group->setText(0, "All particles");
		_tree->addTopLevelItem(group);
		_tree->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(_tree, &QTreeWidget::customContextMenuRequested,
		        this, &View::contextMenu);
		
		connect(_tree, &QTreeWidget::currentItemChanged,
		        this, &View::accessRandomItem);

		bbox->addWidget(_tree);
	}

	{
		QVBoxLayout *vbox = new QVBoxLayout();

		{
			QHBoxLayout *hbox = new QHBoxLayout();

			QVBoxLayout *vvbox = new QVBoxLayout();

			QLabel *l = new QLabel(this);
			l->setObjectName("image");
			MRCImage *im = NULL;
			l->setMinimumSize(512, 512);
			
			if (group != NULL && group->imageCount() > 0)
			{
				im = group->image(0);
				l->setPixmap(QPixmap::fromImage(im->scaled(512, 512)));
			}

			hbox->addWidget(l);

			_finger = new Rotless(this, NULL);
			_finger->useShaders("shaders/quad.vsh", "shaders/finger.fsh");
			_finger->setMinimumSize(512, 512 - 64);
			_finger->setMaximumSize(512, 512 - 64);
			vvbox->addWidget(_finger);

			_rotless = new Rotless(this, NULL);
			_rotless->set1D(true);
			_rotless->useShaders("shaders/quad.vsh", "shaders/twizzle.fsh");
			_rotless->setMinimumSize(512, 64);
			_rotless->setMaximumSize(512, 64);
			vvbox->addWidget(_rotless);
			
			hbox->addLayout(vvbox);

			vbox->addLayout(hbox);
		}

		/*
		{
			QHBoxLayout *hbox = new QHBoxLayout();

			QPushButton *p = new QPushButton("Fingerprint all", this);
			connect(p, &QPushButton::clicked, this, &View::fingerAll);
			hbox->addWidget(p);

			p = new QPushButton("Fingerprint differences", this);
			connect(p, &QPushButton::clicked, this, &View::diffAll);
			hbox->addWidget(p);

			vbox->addLayout(hbox);
		}
		*/
		
		bbox->addLayout(vbox);
	}

	setCentralWidget(window);
	resize(1024, 600);
	show();
	
	/*
	while (getMRC())
	{

	}
	*/
}

bool View::getMRC()
{
	std::string fn = openDialogue(this, "Find MRC file", "*.mrc",
	                              false, false);

	if (fn.length() == 0)
	{
		return false;
	}
	
	addMRCin(fn);
	return true;
}

void View::addMRCin(std::string filename)
{
	MRCin *in = new MRCin(filename);
//	MRCImage *im = in->image(0);

	QTreeWidgetItem *top = _tree->topLevelItem(0);
	ParticleGroup *group = static_cast<ParticleGroup *>(top);
	group->loadFromMRCin(in);
	
	_ins.push_back(in);
}

void View::fingerAll()
{
	QTreeWidgetItem *top = _tree->currentItem();
	if (top == NULL)
	{
		return;
	}

	ParticleGroup *group = static_cast<ParticleGroup *>(top);
	group->fingerprint();

	group->prepareCluster4x(false);

	std::cout << "Done" << std::endl;

	/*
	std::string fn = openDialogue(this, "Save cluster4x file", "*.csv",
	                              true, false);
	
	if (fn.length() == 0)
	{
		return;
	}
	
	QTreeWidgetItem *item = _tree->topLevelItem(0);
	ParticleGroup *grp = dynamic_cast<ParticleGroup *>(item);

	if (grp)
	{
		grp->csv(fn);
	}
	*/
}

void View::accessRandomItem(QTreeWidgetItem *current)
{
	Particle *p = dynamic_cast<Particle *>(current);
	
	if (p != NULL)
	{
		MRCImage *im = p->image();
		QLabel *l = findChild<QLabel *>("image");
		l->setPixmap(QPixmap::fromImage(im->scaled(512, 512)));

		_finger->changeImage(im);
		_rotless->changeImage(im);
	}
}

void View::diffAll()
{
	QTreeWidgetItem *item = _tree->currentItem();
	if (item == NULL)
	{
		return;
	}

	ParticleGroup *grp = dynamic_cast<ParticleGroup *>(item);
	
	if (grp == NULL)
	{
		return;
	}
	
	grp->makeDifferences();

	/*
	std::string fn = openDialogue(this, "Save cluster4x file", "*.csv",
	                              true, false);
	*/
	std::string fn = "";
	
	grp->prepareCluster4x(true);
	
	if (fn.length() == 0)
	{
		return;
	}
	
	grp->csv(fn, true);
}


void View::contextMenu(const QPoint &p)
{
	QMenu *m = new QMenu();
	
	QTreeWidgetItem *item = _tree->currentItem();
	
	if (dynamic_cast<ParticleGroup *>(item))
	{
		QAction *act = m->addAction("Calculate fingerprints");
		connect(act, &QAction::triggered, this, &View::fingerAll);
		act = m->addAction("Differences to cluster4x");
		connect(act, &QAction::triggered, this, &View::diffAll);
	}

	QPoint tmp = p;
	tmp.setY(tmp.y() + menuBar()->height());
	m->exec(mapToGlobal(tmp));
}
