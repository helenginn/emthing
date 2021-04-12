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

#include "DensityDisplay.h"
#include "Trace.h"
#include "Aligner.h"
#include <vaggui/Density2GL.h>
#include <libsrc/FFT.h>
#include <libsrc/PDBReader.h>
#include <libsrc/Crystal.h>
#include "DistortMap.h"
#include "Control.h"
#include "MRCin.h"
#include <h3dsrc/Icosahedron.h>
#include <hcsrc/FileReader.h>
#include <hcsrc/maths.h>
#include <QMenu>
#include <iostream>
#include <fstream>

DensityDisplay::DensityDisplay(QWidget *parent) : SlipGL(parent)
{
	_allRefs = false;
	_activeTrace = NULL;
	_align = NULL;
	_ico = new Icosahedron();
	setBackground(1, 1, 1.0, 1);
	_ico->setColour(0.2, 0.2, 0.2);
	_ico->setPosition(make_vec3(0, 0, 0));
	addObject(_ico, true);
	setZFar(1000);
	_worker = NULL;
}

void DensityDisplay::addDensity(DistortMapPtr vagfft)
{
	_density = new Density2GL();
	_density->setDims(30, 30, 30);
	_density->setResolution(1.0);

	for (size_t i = 0; i < 40; i++)
	{
		_density->nudgeDensity(1);
	}
	
	float h = (360 * 1.605 * _densities.size());
	while (h > 360) h -= 360;
	float s = 80;
	float v = 100;
	hsv_to_rgb(h, s, v);
	vagfft->setColour(h, s, v);

	_density->setDefaultColour(h, s, v);
	_density->makeNewDensity(vagfft);
	addObject(&*vagfft);

	addObject(_density);
	_densities.push_back(_density);
	_ffts.push_back(vagfft);

	if (_ffts.size() >= 2)
	{
		vagfft->setReference(_ffts[0]);
	}
	
	if (vagfft->hasAuxiliary())
	{
		vagfft->loadFromAuxiliary();
	}
	else
	{
		mat3x3 toReal = vagfft->toReal();
		mat3x3 target = _ffts[0]->toReal();
		
		vec3 targo = _ffts[0]->origin();
		vec3 centre_of_target = make_vec3(target.vals[0] / 2,
		                                  target.vals[4] / 2,
		                                  target.vals[8] / 2);
		vec3_add_to_vec3(&centre_of_target, targo);
		
		vec3 myo = vagfft->origin();
		vec3 my_centre = make_vec3(toReal.vals[0] / 2,
		                           toReal.vals[4] / 2,
		                           toReal.vals[8] / 2);
		vec3_add_to_vec3(&my_centre, myo);

		vec3 oriplus = vec3_subtract_vec3(centre_of_target, my_centre);

		vagfft->addToOrigin(oriplus);
	}
}

void DensityDisplay::correctIco()
{
	vec3 c = getCentre();
	mat4x4 inv = mat4x4_inverse(_model);
	c = mat4x4_mult_vec(inv, c);
	_ico->setPosition(c);
}

void DensityDisplay::mouseMoveEvent(QMouseEvent *e)
{
	SlipGL::mouseMoveEvent(e);

	correctIco();
}

void DensityDisplay::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() == Qt::RightButton && !_moving)
	{
		rightClickMenu(e->globalPos());
	}

	SlipGL::mouseReleaseEvent(e);
	correctIco();
}

void DensityDisplay::rightClickMenu(QPoint p)
{
	return;
	QMenu *menu = new QMenu;

	menu->exec(p);
}

void DensityDisplay::done()
{
	disconnect(this, SIGNAL(begin()), nullptr, nullptr);
	disconnect(_align, SIGNAL(resultReady()), nullptr, nullptr);
	
	_worker->quit();
	
	if (_align != NULL)
	{
		if (_allRefs)
		{
			DistortMapPtr ref = _align->reference();
			std::string name = _ref2Name[ref];
			
			std::cout << "RESULTS for " << _align->altered()->filename() << 
			" against " << name << ":" << std::endl;

			for (size_t i = 0; i < _align->symScoreCount(); i++)
			{
				std::cout << "\tSymmetry operator " << i + 1 << " has "
				"correlation of " << _align->symScore(i) << std::endl;
			}
			
			double ratio = _align->symRatio();
			std::cout << "Best score: " << ratio << std::endl;
			std::cout << std::endl;

			_scores[name] = ratio;
		}

		if (_alignables.size() > 0)
		{
			_alignables.erase(_alignables.begin());
		}
		
		if (_allRefs && _alignables.size() == 0)
		{
			std::cout << "Final result: " << std::endl;
			double best = 0;
			int num = 0;
			for (size_t i = 0; i < _refs.size(); i++)
			{
				std::string name = _ref2Name[_refs[i]];
				if (_scores[name] > best)
				{
					best = _scores[name];
					num = i;
				}
			}

			std::cout << _align->altered()->filename() << " matches " <<
			_ref2Name[_refs[num]] << " best." << std::endl;
			std::cout << std::endl;
			_allRefs = false;
		}

		removeObject(_align);
		delete _align;
		_align = NULL;
		startNextAlignment();
	}
}

void DensityDisplay::startNextAlignment()
{
	std::cout << "Alignments left to do: " << _alignables.size() << std::endl;
	if (_alignables.size() == 0)
	{
		_dictator->incrementJob();
		return;
	}

	_align = _alignables[0];
	_objects.insert(_objects.begin(), _align);
	prepareWorkForObject(_align);
	
	connect(this, SIGNAL(begin()), _align, SLOT(align()));
	connect(_align, SIGNAL(resultReady()), this, SLOT(done()));
	connect(_align, SIGNAL(focusOnCentre()), this, SLOT(centre()));
	_worker->start();

	emit begin();
}

void DensityDisplay::alignLastTo(std::string name)
{
	DistortMapPtr ref;
	if (name == "" && _fft2Ref.count(_ffts.back()))
	{
		ref = _fft2Ref[_ffts.back()];
	}
	else if (name == "" && _refs.size() > 0)
	{
		ref = _refs[0];
	}
	else if (name == "" && _refs.size() == 0)
	{
		ref = _ffts[0];
	}
	else
	{
		ref = _name2Ref[name];
	}

	if (_ffts.size() == 0)
	{
		std::cout << "No non-reference FFTs" << std::endl;
		return;
	}
	
	focusOnPosition(empty_vec3(), 100);
	
	if (!ref)
	{
		std::cout << "Reference by name " << name << " does not exist, "
		"declare a reference first after loading a map." << std::endl;
		exit(1);
	}
	
	_align = new Aligner(ref, _ffts.back());
	_align->setSymmetry(_symFile);
	_alignables.push_back(_align);
	
	startNextAlignment();
}

void DensityDisplay::findReferenceBySymmetry()
{
	std::cout << "Comparing to references: " << std::endl;
	for (size_t i = 0; i < _refs.size(); i++)
	{
		DistortMapPtr map = DistortMapPtr(new DistortMap(*_ffts.back()));
		_align = new Aligner(_refs[i], map);
		_align->setSymmetry(_symFile);
		std::cout << _ref2Name[_refs[i]] << ", " << std::endl;

		_alignables.push_back(_align);
	}
	std::cout << "..." << std::endl;

	_allRefs = true;
	startNextAlignment();
}

void DensityDisplay::centre()
{
	vec3 frac = _ffts[0]->fractionalCentroid();
	mat3x3 toReal = _ffts[0]->toReal();
	mat3x3_mult_vec(toReal, &frac);
	vec3 ori = _ffts[0]->origin();
	vec3_add_to_vec3(&frac, ori);
	focusOnPosition(frac, 20);
}

bool DensityDisplay::prepareWorkForObject(QObject *object)
{
	if (object == NULL)
	{
		return false;
	}

	if (_worker && _worker->isRunning())
	{
		std::cout << "Waiting for worker to finish old job first" << std::endl;
		_worker->wait();
	}
	
	if (!_worker)
	{
		_worker = new QThread();
	}

	object->moveToThread(_worker);

	return true;
}

void DensityDisplay::writeLast(std::string filename)
{
	bool dist = (Control::valueForKey("write-distortion") == "true");
	_ffts.back()->writeMRC(filename, dist);
}

void DensityDisplay::correlateLast()
{
	double cor = _ffts.back()->rotateRoundCentre(make_mat3x3(), 
	                                              empty_vec3(), _ffts[0]); 

	std::cout << "Correlation with reference: " << cor << std::endl;
}

void DensityDisplay::correlateAll(std::string prefix)
{
	std::string distname = prefix + "_dist_cc.csv";
	std::ofstream maps, dist;
	dist.open(distname);

	for (size_t i = 1; i < _ffts.size() - 1; i++)
	{
		for (size_t j = i + 1; j < _ffts.size(); j++)
		{
			/*
			double mapcor = _ffts[j]->rotateRoundCentre(make_mat3x3(), 
			                                             empty_vec3(), _ffts[i]); 

			maps << _ffts[i]->filename() << "," << _ffts[j]->filename() << ","
			<< mapcor << std::endl;
			*/

			_ffts[j]->setReference(_ffts[0]);
			double distcor = _ffts[j]->correlateKeypoints(_ffts[i]);

			dist << _ffts[i]->filename() << "," << _ffts[j]->filename() << ","
			<< distcor << std::endl;
		}
	}

	dist.close();
}

void DensityDisplay::pictureLast(std::string filename)
{
	_ffts.back()->drawSlice(-1, filename);
}

void DensityDisplay::dropData()
{
	_ffts.back()->dropData();
}

void DensityDisplay::maskWithPDB(std::string pdb)
{
	PDBReader reader;
	reader.setFilename(pdb);
	CrystalPtr crystal = reader.getCrystal();

	_ffts.back()->maskWithAtoms(crystal);
}

void DensityDisplay::loadFromFileList(std::string filename)
{
	std::string contents = get_file_contents(filename);
	std::vector<std::string> files = split(contents, '\n');
	
	DistortMapPtr map = DistortMapPtr(new DistortMap(0, 0, 0, 0, 0));
	
	for (size_t i = 0; i < files.size(); i++)
	{
		std::cout << "Loading " << files[i] << std::endl;
		DistortMapPtr fft = DistortMapPtr(new DistortMap(0, 0, 0, 0, 0));
		fft->setFilename(files[i]);
		std::string auxfile = getBaseFilenameWithPath(files[i]) + ".aux";
		fft->setReference(_ffts[0]);
		fft->setAuxiliary(auxfile);
		fft->loadFromAuxiliary();
		
		_ffts.push_back(fft);
	}
}

void DensityDisplay::setAsReference(std::string name)
{
	if (_ffts.size() == 0)
	{
		std::cout << "Want to set last density as reference, but "
		"no maps loaded!" << std::endl;
		return;
	}

	DistortMapPtr fft = _ffts.back();
	_ffts.pop_back();
	_refs.push_back(fft);
	
	_name2Ref[name] = fft;
	_ref2Name[fft] = name;
}
