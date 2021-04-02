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

#include "ParticleGroup.h"
#include "Particle.h"
#include "View.h"
#include "MRCin.h"
#include "Rotless.h"
#include "MRCImage.h"

#include <hcsrc/FileReader.h>

#include <c4xsrc/ClusterList.h>
#include <c4xsrc/AveCSV.h>
#include <c4xsrc/Group.h>

#include <fstream>
#include <iostream>
#include <algorithm>

#include <QLabel>

ParticleGroup::ParticleGroup(QTreeWidget *parent) : QTreeWidgetItem(parent)
{
	_screen = NULL;
	_view = NULL;
	_tree = parent;
}

ParticleGroup::ParticleGroup(ParticleGroup *parent) : QTreeWidgetItem(parent)
{
	_screen = NULL;
	_tree = parent->_tree;
	_view = parent->_view;
}

void ParticleGroup::loadFromMRCin(MRCin *mrcin)
{
	for (size_t i = 0; i < mrcin->imageCount(); i++)
	{
		MRCImage *im = mrcin->image(i);
		addImage(im);
	}
}

void ParticleGroup::makeDifferences()
{
	fingerprint();

	for (int i = 0; i < imageCount(); i++)
	{
		MRCImage *mp = static_cast<Particle *>(child(i))->image();
		mp->allocateDifferprint(imageCount());
		
		for (int j = 0; j < imageCount(); j++)
		{
			MRCImage *mq = static_cast<Particle *>(child(j))->image();
			mp->populateDifferprint(j, mq);
		}
	}
}

void ParticleGroup::csv(std::string filename, bool diff)
{
	std::ofstream file;
	file.open(filename);
	
	for (size_t i = 0; i < imageCount(); i++)
	{
		MRCImage *ima = image(i);
		for (size_t j = 0; j < i; j++)
		{
			MRCImage *imb = image(j);
			double correl = 0;
			
			if (diff)
			{
				correl = ima->diffCorrelWith(imb);
			}
			else
			{
				correl = ima->correlWith(imb);
			}

			file << ima->name() << "," << imb->name() << 
			"," << correl << std::endl;

			if (i < 3 && j < 3)
			{
				std::cout << ima->name() << "," << imb->name() << 
				"," << correl << std::endl;
			}
		}
	}
	
	file.close();
}

std::vector<MRCImage *> ParticleGroup::images()
{
	std::vector<MRCImage *> images;
	for (size_t i = 0; i < imageCount(); i++)
	{
		images.push_back(image(i));
	}
	
	return images;
}

void ParticleGroup::prepareCluster4x(bool diffs)
{
	if (_screen != NULL)
	{
		_screen->hide();
		_screen->deleteLater();
	}
	_screen = new Screen(NULL);
	_screen->setWindowTitle("cluster4x - emthing");
	_screen->setReturnJourney(this);

	AveCSV *csv = new AveCSV(NULL, "");
	ClusterList *list = _screen->getList();
	csv->setList(list);
	csv->startNewCSV("Particle similarity");
	
	std::vector<MRCImage *> copy = images();
	
	std::random_shuffle(copy.begin(), copy.end());
	
	size_t limit = 1000 + 1;
	if (limit > copy.size())
	{
		limit = copy.size();
	}

	for (size_t i = 1; i < limit - 1; i++)
	{
		for (size_t j = i + 1; j < limit; j++)
		{
			double correl = 0;
			
			if (diffs)
			{
				correl = copy[i]->diffCorrelWith(copy[j]);
			}
			else
			{
				correl = copy[i]->correlWith(copy[j]);
			}

			csv->addValue(copy[i]->name(), copy[j]->name(), correl);
			csv->addValue(copy[j]->name(), copy[i]->name(), correl);
		}
	}

	csv->preparePaths();
	csv->setChosen(0);

	Group *top = Group::topGroup();
	top->setCustomName(text(0).toStdString());
	top->updateText();

	_screen->show();
}

void ParticleGroup::finished()
{
	ClusterList *list = _screen->getList();

	for (size_t i = 0; i < list->groupCount(); i++)
	{
		Group *g = list->group(i);
		
		if (!g->isMarked())
		{
			continue;
		}

		ParticleGroup *grp = new ParticleGroup(_tree);
		grp->setView(_view);
		std::string def = "Split " + i_to_str(i);
		
		std::string custom = g->getCustomName();
		if (custom.length())
		{
			def = custom;
		}

		for (size_t j = 0; j < g->mtzCount(); j++)
		{
			std::string imageName = g->getMetadata(j);
			MRCImage *image = _nameMap[imageName];
			grp->addImage(image);
		}

		grp->setText(0, QString::fromStdString(def));
		
		_tree->addTopLevelItem(grp);
		_tree->setCurrentItem(grp);
	}
	
	_screen->hide();
}


void ParticleGroup::addImage(MRCImage *image)
{
	_images.push_back(image);
	_nameMap[image->name()] = image;
	Particle *p = new Particle(image);
	addChild(p);
}

size_t ParticleGroup::imageCount()
{
	return (size_t)_images.size();
}


MRCImage *ParticleGroup::image(int i)
{
	return _images[i];
}

void ParticleGroup::fingerprint()
{
	for (size_t i = 0; i < imageCount(); i++)
	{
		MRCImage *im = image(i);
		
		if (im->hasFingerprint())
		{
			continue;
		}

		_view->finger()->changeImage(im);
		_view->twizzle()->changeImage(im);
		
		QImage mi = _view->finger()->grabFramebuffer();
		
		std::cout << i << " / " << imageCount() << std::endl;
		
		if (i == 10)
		{
			im->writeImage();
		}
	}
}
