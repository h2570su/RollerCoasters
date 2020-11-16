/************************************************************************
	 File:        TrainWindow.H

	 Author:
				  Michael Gleicher, gleicher@cs.wisc.edu

	 Modifier
				  Yu-Chi Lai, yu-chi@cs.wisc.edu

	 Comment:
						this class defines the window in which the project
						runs - its the outer windows that contain all of
						the widgets, including the "TrainView" which has the
						actual OpenGL window in which the train is drawn

						You might want to modify this class to add new widgets
						for controlling	your train

						This takes care of lots of things - including installing
						itself into the FlTk "idle" loop so that we get periodic
						updates (if we're running the train).


	 Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <FL/fl.h>
#include <FL/Fl_Box.h>
#include <string>

// for using the real time clock
#include <time.h>

#include "TrainWindow.H"
#include "TrainView.H"
#include "CallBacks.H"



//************************************************************************
//
// * Constructor
//========================================================================
TrainWindow::
TrainWindow(const int x, const int y)
	: Fl_Double_Window(x, y, 800, 600, "Train and Roller Coaster")
	//========================================================================
{
	// make all of the widgets
	begin();	// add to this widget
	{
		int pty = 5;			// where the last widgets were drawn

		trainView = new TrainView(5, 5, 590, 590);
		trainView->tw = this;
		trainView->m_pTrack = &m_Track;
		this->resizable(trainView);

		// to make resizing work better, put all the widgets in a group
		widgets = new Fl_Group(600, 5, 190, 590);
		widgets->begin();

		runButton = new Fl_Button(605, pty, 60, 20, "Run");
		togglify(runButton);

		Fl_Button* fb = new Fl_Button(700, pty, 25, 20, "@>>");
		fb->callback((Fl_Callback*)forwCB, this);
		Fl_Button* rb = new Fl_Button(670, pty, 25, 20, "@<<");
		rb->callback((Fl_Callback*)backCB, this);

		arcLength = new Fl_Button(730, pty, 65, 20, "ArcLength");
		togglify(arcLength, 1);

		pty += 25;
		speed = new Fl_Value_Slider(655, pty, 140, 20, "speed");
		speed->range(0, 10);
		speed->value(2);
		speed->align(FL_ALIGN_LEFT);
		speed->type(FL_HORIZONTAL);

		pty += 30;

		// camera buttons - in a radio button group
		Fl_Group* camGroup = new Fl_Group(600, pty, 195, 20);
		camGroup->begin();
		worldCam = new Fl_Button(605, pty, 60, 20, "World");
		worldCam->type(FL_RADIO_BUTTON);		// radio button
		worldCam->value(1);			// turned on
		worldCam->selection_color((Fl_Color)3); // yellow when pressed
		worldCam->callback((Fl_Callback*)damageCB, this);
		trainCam = new Fl_Button(670, pty, 60, 20, "Train");
		trainCam->type(FL_RADIO_BUTTON);
		trainCam->value(0);
		trainCam->selection_color((Fl_Color)3);
		trainCam->callback((Fl_Callback*)damageCB, this);
		topCam = new Fl_Button(735, pty, 60, 20, "Top");
		topCam->type(FL_RADIO_BUTTON);
		topCam->value(0);
		topCam->selection_color((Fl_Color)3);
		topCam->callback((Fl_Callback*)damageCB, this);
		camGroup->end();

		pty += 30;

		// browser to select spline types
		// TODO: make sure these choices are the same as what the code supports
		splineBrowser = new Fl_Browser(605, pty, 120, 75, "Spline Type");
		splineBrowser->type(2);		// select
		splineBrowser->callback((Fl_Callback*)damageCB, this);
		splineBrowser->add("Linear");
		splineBrowser->add("Cardinal Cubic");
		splineBrowser->add("Cubic B-Spline");
		splineBrowser->select(2);

		pty += 110;

		// add and delete points
		Fl_Button* ap = new Fl_Button(605, pty, 80, 20, "Add Point");
		ap->callback((Fl_Callback*)addPointCB, this);
		Fl_Button* dp = new Fl_Button(690, pty, 80, 20, "Delete Point");
		dp->callback((Fl_Callback*)deletePointCB, this);

		pty += 25;
		// reset the points
		resetButton = new Fl_Button(735, pty, 60, 20, "Reset");
		resetButton->callback((Fl_Callback*)resetCB, this);
		Fl_Button* loadb = new Fl_Button(605, pty, 60, 20, "Load");
		loadb->callback((Fl_Callback*)loadCB, this);
		Fl_Button* saveb = new Fl_Button(670, pty, 60, 20, "Save");
		saveb->callback((Fl_Callback*)saveCB, this);

		pty += 25;
		// roll the points
		Fl_Button* rx = new Fl_Button(605, pty, 30, 20, "R+X");
		rx->callback((Fl_Callback*)rpxCB, this);
		Fl_Button* rxp = new Fl_Button(635, pty, 30, 20, "R-X");
		rxp->callback((Fl_Callback*)rmxCB, this);
		Fl_Button* rz = new Fl_Button(670, pty, 30, 20, "R+Z");
		rz->callback((Fl_Callback*)rpzCB, this);
		Fl_Button* rzp = new Fl_Button(700, pty, 30, 20, "R-Z");
		rzp->callback((Fl_Callback*)rmzCB, this);

		pty += 30;


		// TODO: add widgets for all of your fancier features here
		physics = new Fl_Button(605, pty, 65, 20, "Physics");
		togglify(physics, 1);
		

		pty += 30;

		currentSpeed = new Fl_Box(605, pty, 0, 0, "Current Speed: 0.0000");
		currentSpeed->align(FL_ALIGN_RIGHT);
		pty += 30;

		incCart = new Fl_Button(605, pty, 20, 20, "+");
		incCart->callback((Fl_Callback*)incCartCB, this);
		decCart = new Fl_Button(700, pty, 20, 20, "-");
		decCart->callback((Fl_Callback*)decCartCB, this);
		std::string cartCountStr = "Carts: 5";
		strcpy_s(this->currentCartCountStr, cartCountStr.c_str());
		CartCount = new Fl_Box(650, pty, 20, 20, this->currentCartCountStr);
		
		pty += 30;
		multiThread = new Fl_Button(605, pty, 100, 20, "Multi Threading");
		togglify(multiThread, 1);

		// we need to make a little phantom widget to have things resize correctly
		Fl_Box* resizebox = new Fl_Box(600, 595, 200, 5);
		widgets->resizable(resizebox);

		widgets->end();
	}
	end();	// done adding to this widget

	// set up callback on idle
	Fl::add_idle((void(*)(void*))runButtonCB, this);
}

//************************************************************************
//
// * handy utility to make a button into a toggle
//========================================================================
void TrainWindow::
togglify(Fl_Button* b, int val)
//========================================================================
{
	b->type(FL_TOGGLE_BUTTON);		// toggle
	b->value(val);		// turned off
	b->selection_color((Fl_Color)3); // yellow when pressed	
	b->callback((Fl_Callback*)damageCB, this);
}

//************************************************************************
//
// *
//========================================================================
void TrainWindow::
damageMe()
//========================================================================
{
	if (trainView->selectedCube >= ((int)m_Track.points.size()))
		trainView->selectedCube = 0;
	trainView->damage(1);
}

//************************************************************************
//
// * This will get called (approximately) 30 times per second
//   if the run button is pressed
//========================================================================
void TrainWindow::
advanceTrain(float dir)
//========================================================================
{
	//#####################################################################
	// TODO: make this work for your train
	//#####################################################################

	// note - we give a little bit more example code here than normal,
	// so you can see how this works
	int type = 0;
	type = (this->splineBrowser->selected(1)) ? 1 : type;
	type = (this->splineBrowser->selected(2)) ? 2 : type;
	type = (this->splineBrowser->selected(3)) ? 3 : type;

	if (arcLength->value())
	{
		double targetMovement;
		if (this->physics->value())
		{
			Pnt3f cDir;
			this->trainView->getDir(this->m_Track.trainU, cDir, type);
			this->trainView->currTrainSpeed += (-cDir.y)*this->trainView->gravityFactor;
			this->trainView->currTrainSpeed = (this->trainView->currTrainSpeed < this->trainView->minSpeed) ? this->trainView->minSpeed : this->trainView->currTrainSpeed;
			this->trainView->currTrainSpeed = (this->trainView->currTrainSpeed > this->trainView->maxSpeed) ? this->trainView->maxSpeed : this->trainView->currTrainSpeed;
			targetMovement = this->trainView->currTrainSpeed * ((float)speed->value() * .05f) * dir;
			std::string str = ("Current Speed: " + std::to_string(this->trainView->currTrainSpeed));
			strcpy_s(this->currentSpeedStr, str.c_str());
			this->currentSpeed->label(this->currentSpeedStr);
		}
		else
		{
			targetMovement = this->trainView->defaultSpeed * ((float)speed->value() * .05f) * dir;
			std::string str = ("Current Speed: " + std::to_string(this->trainView->defaultSpeed));
			strcpy_s(this->currentSpeedStr, str.c_str());
			this->currentSpeed->label(this->currentSpeedStr);
		}

		double sumMovement = 0;
		Pnt3f pv;
		this->trainView->getPos(this->m_Track.trainU, pv, type);
		double tInc = (1.0 / this->trainView->DIVIDE_LINE);
		if (signbit(targetMovement))
		{
			tInc *= -1;
		}
		for (int i = 0; i < this->trainView->DIVIDE_LINE; i++)
		{
			if (fabs(sumMovement) >= fabs(targetMovement))
			{
				break;
			}
			Pnt3f cv;
			this->m_Track.trainU += tInc;
			this->trainView->getPos(this->m_Track.trainU, cv, type);
			sumMovement += sqrt((cv.x - pv.x)*(cv.x - pv.x) + (cv.y - pv.y)*(cv.y - pv.y) + (cv.z - pv.z)*(cv.z - pv.z));
			pv = cv;

		}
	}
	else
	{
		this->m_Track.trainU += dir * ((float)speed->value() * .05f);
	}
	float nct = this->m_Track.points.size();
	if (this->m_Track.trainU > nct) this->m_Track.trainU -= nct;
	if (this->m_Track.trainU < 0) this->m_Track.trainU += nct;
}