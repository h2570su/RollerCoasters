/************************************************************************
	 File:        TrainView.cpp

	 Author:
				  Michael Gleicher, gleicher@cs.wisc.edu

	 Modifier
				  Yu-Chi Lai, yu-chi@cs.wisc.edu

	 Comment:
						The TrainView is the window that actually shows the
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within
						a TrainWindow
						that is the outer window with all the widgets.
						The TrainView needs
						to be aware of the window - since it might need to
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know
						about it (beware circular references)

	 Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GL/glu.h"

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"
#include <algorithm>
#include <ppl.h>
#include <mutex>

#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif


//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l)
	: Fl_Gl_Window(x, y, w, h, l)
	//========================================================================
{
	mode(FL_RGB | FL_ALPHA | FL_DOUBLE | FL_STENCIL);

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event))
			return 1;

	// remember what button was used
	static int last_push;

	switch (event) {
		// Mouse button being pushed event
	case FL_PUSH:
		last_push = Fl::event_button();
		// if the left button be pushed is left mouse button
		if (last_push == FL_LEFT_MOUSE) {
			doPick();
			damage(1);
			return 1;
		};
		break;

		// Mouse button release event
	case FL_RELEASE: // button release
		damage(1);
		last_push = 0;
		return 1;

		// Mouse button drag event
	case FL_DRAG:

		// Compute the new control point position
		if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
			ControlPoint* cp = &m_pTrack->points[selectedCube];

			double r1x, r1y, r1z, r2x, r2y, r2z;
			getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

			double rx, ry, rz;
			mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z,
				static_cast<double>(cp->pos.x),
				static_cast<double>(cp->pos.y),
				static_cast<double>(cp->pos.z),
				rx, ry, rz,
				(Fl::event_state() & FL_CTRL) != 0);

			cp->pos.x = (float)rx;
			cp->pos.y = (float)ry;
			cp->pos.z = (float)rz;
			damage(1);
		}
		break;

		// in order to get keyboard events, we need to accept focus
	case FL_FOCUS:
		return 1;

		// every time the mouse enters this window, aggressively take focus
	case FL_ENTER:
		focus(this);
		break;

	case FL_KEYBOARD:
		int k = Fl::event_key();
		int ks = Fl::event_state();
		if (k == 'p') {
			// Print out the selected control point information
			if (selectedCube >= 0)
				printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
					selectedCube,
					m_pTrack->points[selectedCube].pos.x,
					m_pTrack->points[selectedCube].pos.y,
					m_pTrack->points[selectedCube].pos.z,
					m_pTrack->points[selectedCube].orient.x,
					m_pTrack->points[selectedCube].orient.y,
					m_pTrack->points[selectedCube].orient.z);
			else
				printf("Nothing Selected\n");

			return 1;
		};
		break;
	}

	return Fl_Gl_Window::handle(event);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0, 0, w(), h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0, 0, .3f, 0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	}
	else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	GLfloat lightPosition1[] = { 0,1,1,0 }; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[] = { 1, 0, 0, 0 };
	GLfloat lightPosition3[] = { 0, -1, 0, 0 };
	GLfloat yellowLight[] = { 0.5f, 0.5f, .1f, 1.0 };
	GLfloat whiteLight[] = { .5f, .5f, .5f, 1.0 };
	GLfloat blueLight[] = { .1f,.1f,.3f,1.0 };
	GLfloat grayLight[] = { .15f, .15f, .15f, 1.0 };

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);

	GLfloat noLight[] = { 0,0,0,1 };
	GLfloat spotDir[] = { 0,-1,0 };

	GLfloat light3Pos[] = { -50,200,-50,1 };
	GLfloat light3ColorDeep[] = { 0,0.2,0,1 };
	GLfloat light3ColorLight[] = { 0,0.4,0,1 };
	glEnable(GL_LIGHT3);
	glLightfv(GL_LIGHT3, GL_POSITION, light3Pos);
	glLightfv(GL_LIGHT3, GL_AMBIENT, light3ColorDeep);
	glLightfv(GL_LIGHT3, GL_DIFFUSE, light3ColorLight);
	glLightfv(GL_LIGHT3, GL_SPECULAR, light3ColorLight);


	glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, spotDir);
	glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 30);
	glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 10.0f);

	GLfloat light4Pos[] = { 50,200,50,1 };
	GLfloat light4ColorDeep[] = { 0.3,0,0,1 };
	GLfloat light4ColorLight[] = { 0.6,0,0,1 };
	glEnable(GL_LIGHT4);
	glLightfv(GL_LIGHT4, GL_POSITION, light4Pos);
	glLightfv(GL_LIGHT4, GL_AMBIENT, light4ColorDeep);
	glLightfv(GL_LIGHT4, GL_DIFFUSE, light4ColorLight);
	glLightfv(GL_LIGHT4, GL_SPECULAR, light4ColorLight);


	glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, spotDir);
	glLightf(GL_LIGHT4, GL_SPOT_CUTOFF, 30);
	glLightf(GL_LIGHT4, GL_SPOT_EXPONENT, 10.0f);

	GLfloat light5Pos[] = { -50,200,50,1 };
	GLfloat light5ColorDeep[] = { 0,0,0.35,1 };
	GLfloat light5ColorLight[] = { 0,0,0.7,1 };
	glEnable(GL_LIGHT5);
	glLightfv(GL_LIGHT5, GL_POSITION, light5Pos);
	glLightfv(GL_LIGHT5, GL_AMBIENT, light5ColorDeep);
	glLightfv(GL_LIGHT5, GL_DIFFUSE, light5ColorLight);
	glLightfv(GL_LIGHT5, GL_SPECULAR, light5ColorLight);


	glLightfv(GL_LIGHT5, GL_SPOT_DIRECTION, spotDir);
	glLightf(GL_LIGHT5, GL_SPOT_CUTOFF, 30);
	glLightf(GL_LIGHT5, GL_SPOT_EXPONENT, 10.0f);

	GLfloat light6Pos[] = { 50,200,-50,1 };
	GLfloat light6ColorDeep[] = { 0.3,0.2,0,1 };
	GLfloat light6ColorLight[] = { 0.6,0.4,0,1 };
	glEnable(GL_LIGHT6);
	glLightfv(GL_LIGHT6, GL_POSITION, light6Pos);
	glLightfv(GL_LIGHT6, GL_AMBIENT, light6ColorDeep);
	glLightfv(GL_LIGHT6, GL_DIFFUSE, light6ColorLight);
	glLightfv(GL_LIGHT6, GL_SPECULAR, light6ColorLight);


	glLightfv(GL_LIGHT6, GL_SPOT_DIRECTION, spotDir);
	glLightf(GL_LIGHT6, GL_SPOT_CUTOFF, 30);
	glLightf(GL_LIGHT6, GL_SPOT_EXPONENT, 10.0f);



	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	//glDisable(GL_LIGHTING);
	drawFloor(200, 10);


	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
	{
		arcball.setProjection(false);
	}
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		}
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90, 1, 0, 0);
	}
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
		//arcball.setup(this, 40, 250, .2f, .4f, 0);
		Pnt3f pos, dir, up;
		int type = 0;
		type = (this->tw->splineBrowser->selected(1)) ? 1 : type;
		type = (this->tw->splineBrowser->selected(2)) ? 2 : type;
		type = (this->tw->splineBrowser->selected(3)) ? 3 : type;
		this->getPos(this->tw->m_Track.trainU, pos, type);
		this->getDir(this->tw->m_Track.trainU, dir, type);
		this->getOrient(this->tw->m_Track.trainU, up, type);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(70, aspect, 0.1, 1000);


		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		pos = pos + up * (trainHeight / 2) + dir * (trainLength*1.1 / 2);

		Pnt3f lookat = pos + dir;
		lookat = lookat * 1;

		gluLookAt(pos.x, pos.y, pos.z,
			lookat.x, lookat.y, lookat.z,
			up.x, up.y, up.z);
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################


//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if (((int)i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################

	int type = 0;
	type = (this->tw->splineBrowser->selected(1)) ? 1 : type;
	type = (this->tw->splineBrowser->selected(2)) ? 2 : type;
	type = (this->tw->splineBrowser->selected(3)) ? 3 : type;
	if (!this->tw->trainCam->value())
	{
		this->drawCarts(this->m_pTrack->trainU, doingShadows);
	}
	if (this->cartsCount > 0)
	{
		double currCartT = this->m_pTrack->trainU;

		for (int c = 0; c < this->cartsCount; c++)
		{
			double cartMoveSum = 0;
			Pnt3f pv;


			this->getPos(currCartT, pv, type);
			double incT = -1.0 / (this->DIVIDE_LINE);
			for (int i = 0; i < this->DIVIDE_LINE; i++)
			{
				if (cartMoveSum >= this->cartsSpacing)
				{
					break;
				}
				Pnt3f cv;
				currCartT += incT;
				this->getPos(currCartT, cv, type);
				cartMoveSum += sqrt((cv.x - pv.x)*(cv.x - pv.x) + (cv.y - pv.y)*(cv.y - pv.y) + (cv.z - pv.z)*(cv.z - pv.z));
				pv = cv;
			}
			this->drawCarts(currCartT, doingShadows);
		}
	}



	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################
	this->trackDrawList.clear();
	this->barDrawList.clear();

	bool arcLengthEnabled = this->tw->arcLength->value();
	if (this->tw->multiThread->value())
	{
		std::mutex g_mutex;
		Concurrency::parallel_for(size_t(0), this->m_pTrack->points.size(), [&](size_t i)
		{
			std::vector<BarNeeds> bars;
			std::vector<TrackNeeds> lines;
			lines.reserve(DIVIDE_LINE * 6);
			Pnt3f pos, dir, up;
			float t = i;

			this->getPos(t, pos, type);
			this->getOrient(t, up, type);
			Pnt3f pv = pos;
			Pnt3f po = up;
			float distSum = 0;
			for (int j = 0; j < DIVIDE_LINE; j++)
			{
				t += 1.0 / DIVIDE_LINE;
				Pnt3f cv;
				Pnt3f co;
				this->getPos(t, cv, type);
				this->getOrient(t, co, type);

				if (arcLengthEnabled)
				{
					distSum += sqrt((cv.x - pv.x)*(cv.x - pv.x) + (cv.y - pv.y)*(cv.y - pv.y) + (cv.z - pv.z)*(cv.z - pv.z));
				}

				Pnt3f cross_t = (cv - pv) * co;
				cross_t.normalize();
				cross_t = cross_t * 2.5f;

				//Track Lines

				lines.push_back(TrackNeeds{ pv,cv,cross_t });

				//Track Bars
				if ((!arcLengthEnabled && j % (this->DIVIDE_LINE / 10) == 0) || (arcLengthEnabled && distSum >= barSpacing))
				{
					distSum = 0;
					getDir(t, dir, type);

					bars.push_back(BarNeeds{ cv, dir,co });
				}
				pv = cv;
			}
			g_mutex.lock();
			this->barDrawList.insert(this->barDrawList.end(), bars.begin(), bars.end());
			this->trackDrawList.insert(this->trackDrawList.end(), lines.begin(), lines.end());
			g_mutex.unlock();

		});

		for (auto& v : this->trackDrawList)
		{
			drawTrack(v.pv, v.cv, v.cross_t, doingShadows);
		}

		for (auto& v : this->barDrawList)
		{
			drawBar(v.pos, v.dir, v.up, doingShadows);
		}
	}
	else
	{
		for (int i = 0; i < this->m_pTrack->points.size(); i++)
		{
			Pnt3f pos, dir, up;
			float t = i;

			this->getPos(t, pos, type);
			this->getOrient(t, up, type);
			Pnt3f pv = pos;
			Pnt3f po = up;
			float distSum = 0;
			for (int j = 0; j < DIVIDE_LINE; j++)
			{
				t += 1.0 / DIVIDE_LINE;
				Pnt3f cv;
				Pnt3f co;
				this->getPos(t, cv, type);
				this->getOrient(t, co, type);

				if (arcLengthEnabled)
				{
					distSum += sqrt((cv.x - pv.x)*(cv.x - pv.x) + (cv.y - pv.y)*(cv.y - pv.y) + (cv.z - pv.z)*(cv.z - pv.z));
				}

				Pnt3f cross_t = (cv - pv) * co;
				cross_t.normalize();
				cross_t = cross_t * 2.5f;

				//Track Lines
				drawTrack(pv, cv, cross_t, doingShadows);

				//Track Bars
				if ((!arcLengthEnabled && j % (this->DIVIDE_LINE / 10) == 0) || (arcLengthEnabled && distSum >= barSpacing))
				{
					distSum = 0;
					getDir(t, dir, type);
					drawBar(cv, dir, co, doingShadows);
				}
				pv = cv;
			}
		}
	}
}

void TrainView::drawTrack(Pnt3f pv, Pnt3f cv, Pnt3f cross_t, bool doingShadows)
{
	if (!doingShadows)
	{

		glColor3ub(32, 32, 64);
	}
	glLineWidth(5);
	glBegin(GL_LINES);
	glVertex3f(pv.x + cross_t.x, pv.y + cross_t.y, pv.z + cross_t.z);
	glVertex3f(cv.x + cross_t.x, cv.y + cross_t.y, cv.z + cross_t.z);
	glVertex3f(pv.x - cross_t.x, pv.y - cross_t.y, pv.z - cross_t.z);
	glVertex3f(cv.x - cross_t.x, cv.y - cross_t.y, cv.z - cross_t.z);
	glEnd();
}

void TrainView::drawBar(Pnt3f pos, Pnt3f dir, Pnt3f up, bool doingShadows)
{
	Pnt3f cv = pos;
	if (!doingShadows)
		glColor3ub(255, 255, 255);
	float BarLengthRatio = 3;
	float BarWidth = 1;
	float BarHeight = 0.5;
	float BarLength = 7.5;

	glPushMatrix();
	glTranslatef(cv.x, cv.y, cv.z);


	Pnt3f u = dir;

	Pnt3f w = u * up;
	w.normalize();
	Pnt3f v = w * u;
	v.normalize();

	float rotation[16] = {
	u.x, u.y, u.z, 0.0,
	v.x, v.y, v.z, 0.0,
	w.x, w.y, w.z, 0.0,
	0.0, 0.0, 0.0, 1.0 };
	glMultMatrixf(rotation);
	glRotatef(90, 0, 1, 0);
	glTranslatef(0, -BarHeight, 0);
	glBegin(GL_QUADS);
	if (!doingShadows)
	{
		glColor3ub(255, 255, 255);
	}

	glNormal3f(0, -1, 0);
	glVertex3f(-BarLength / 2, -BarHeight / 2, -BarWidth / 2);
	glVertex3f(-BarLength / 2, -BarHeight / 2, BarWidth / 2);
	glVertex3f(BarLength / 2, -BarHeight / 2, BarWidth / 2);
	glVertex3f(BarLength / 2, -BarHeight / 2, -BarWidth / 2);


	glNormal3f(-1, 0, 0);
	glVertex3f(-BarLength / 2, BarHeight / 2, -BarWidth / 2);
	glVertex3f(-BarLength / 2, BarHeight / 2, BarWidth / 2);
	glVertex3f(-BarLength / 2, -BarHeight / 2, BarWidth / 2);
	glVertex3f(-BarLength / 2, -BarHeight / 2, -BarWidth / 2);

	glNormal3f(1, 0, 0);
	glVertex3f(BarLength / 2, BarHeight / 2, -BarWidth / 2);
	glVertex3f(BarLength / 2, BarHeight / 2, BarWidth / 2);
	glVertex3f(BarLength / 2, -BarHeight / 2, BarWidth / 2);
	glVertex3f(BarLength / 2, -BarHeight / 2, -BarWidth / 2);


	glNormal3f(0, 0, -1);
	glVertex3f(-BarLength / 2, -BarHeight / 2, -BarWidth / 2);
	glVertex3f(-BarLength / 2, BarHeight / 2, -BarWidth / 2);
	glVertex3f(BarLength / 2, BarHeight / 2, -BarWidth / 2);
	glVertex3f(BarLength / 2, -BarHeight / 2, -BarWidth / 2);


	glNormal3f(0, 0, 1);
	glVertex3f(-BarLength / 2, -BarHeight / 2, BarWidth / 2);
	glVertex3f(-BarLength / 2, BarHeight / 2, BarWidth / 2);
	glVertex3f(BarLength / 2, BarHeight / 2, BarWidth / 2);
	glVertex3f(BarLength / 2, -BarHeight / 2, BarWidth / 2);

	if (!doingShadows)
	{
		glColor3ub(255, 0, 0);
	}

	glNormal3f(0, 1, 0);
	glVertex3f(-BarLength / 2, BarHeight / 2, -BarWidth / 2);
	glVertex3f(-BarLength / 2, BarHeight / 2, BarWidth / 2);
	glVertex3f(BarLength / 2, BarHeight / 2, BarWidth / 2);
	glVertex3f(BarLength / 2, BarHeight / 2, -BarWidth / 2);

	glEnd();
	glPopMatrix();
}



void TrainView::drawCarts(float t, bool doingShadows)
{
	int type = 0;
	type = (this->tw->splineBrowser->selected(1)) ? 1 : type;
	type = (this->tw->splineBrowser->selected(2)) ? 2 : type;
	type = (this->tw->splineBrowser->selected(3)) ? 3 : type;
	Pnt3f pos, dir, up;
	this->getPos(t, pos, type);
	this->getDir(t, dir, type);
	this->getOrient(t, up, type);


	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);

	Pnt3f u = dir;

	Pnt3f w = u * up;
	w.normalize();
	Pnt3f v = w * u;
	v.normalize();

	float rotation[16] = {
	u.x, u.y, u.z, 0.0,
	v.x, v.y, v.z, 0.0,
	w.x, w.y, w.z, 0.0,
	0.0, 0.0, 0.0, 1.0 };
	glMultMatrixf(rotation);
	glRotatef(90, 0, 1, 0);
	glTranslatef(0, trainHeight / 2+wheelRaduis, 0);

#pragma region trainCar
	glBegin(GL_QUADS);
	if (!doingShadows)
	{
		glColor3ub(255, 255, 255);
	}

	glNormal3f(0, -1, 0);
	glVertex3f(-trainWidth / 2, -trainHeight / 2, -trainLength / 2);
	glVertex3f(trainWidth / 2, -trainHeight / 2, -trainLength / 2);
	glVertex3f(trainWidth / 2, -trainHeight / 2, trainLength / 2);
	glVertex3f(-trainWidth / 2, -trainHeight / 2, trainLength / 2);




	glNormal3f(-1, 0, 0);
	glVertex3f(-trainWidth / 2, -trainHeight / 2, -trainLength / 2);
	glVertex3f(-trainWidth / 2, trainHeight / 2, -trainLength / 2);
	glVertex3f(-trainWidth / 2, trainHeight / 2, trainLength / 2);
	glVertex3f(-trainWidth / 2, -trainHeight / 2, trainLength / 2);

	glNormal3f(1, 0, 0);
	glVertex3f(trainWidth / 2, -trainHeight / 2, -trainLength / 2);
	glVertex3f(trainWidth / 2, trainHeight / 2, -trainLength / 2);
	glVertex3f(trainWidth / 2, trainHeight / 2, trainLength / 2);
	glVertex3f(trainWidth / 2, -trainHeight / 2, trainLength / 2);

	glNormal3f(0, 0, -1);
	glVertex3f(-trainWidth / 2, -trainHeight / 2, -trainLength / 2);
	glVertex3f(trainWidth / 2, -trainHeight / 2, -trainLength / 2);
	glVertex3f(trainWidth / 2, trainHeight / 2, -trainLength / 2);
	glVertex3f(-trainWidth / 2, trainHeight / 2, -trainLength / 2);
	if (!doingShadows)
	{
		glColor3ub(0, 255, 0);
	}

	glNormal3f(0, 1, 0);
	glVertex3f(-trainWidth / 2, trainHeight / 2, -trainLength / 2);
	glVertex3f(trainWidth / 2, trainHeight / 2, -trainLength / 2);
	glVertex3f(trainWidth / 2, trainHeight / 2, trainLength / 2);
	glVertex3f(-trainWidth / 2, trainHeight / 2, trainLength / 2);

	if (!doingShadows)
	{
		glColor3ub(255, 0, 0);
	}
	glNormal3f(0, 0, 1);
	glVertex3f(-trainWidth / 2, -trainHeight / 2, trainLength / 2);
	glVertex3f(trainWidth / 2, -trainHeight / 2, trainLength / 2);
	glVertex3f(trainWidth / 2, trainHeight / 2, trainLength / 2);
	glVertex3f(-trainWidth / 2, trainHeight / 2, trainLength / 2);


	glEnd();

	glPushMatrix();
	glTranslatef(trainWidth / 2, -trainHeight / 2, trainLength/2-2);
	drawWheel(doingShadows);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(trainWidth / 2, -trainHeight / 2, 0);
	drawWheel(doingShadows);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(trainWidth / 2, -trainHeight / 2, -trainLength/2+2);
	drawWheel(doingShadows);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-trainWidth / 2-wheelWidth, -trainHeight / 2, trainLength/2-2);
	drawWheel(doingShadows);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-trainWidth / 2-wheelWidth, -trainHeight / 2, 0);
	drawWheel(doingShadows);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-trainWidth / 2-wheelWidth, -trainHeight / 2, -trainLength/2+2);
	drawWheel(doingShadows);
	glPopMatrix();

#pragma endregion
	glPopMatrix();
}

void TrainView::drawWheel(bool doingShadows)
{
	glRotatef(90, 0, 1, 0);
	
	static GLUquadricObj *objCylinder = gluNewQuadric();
	if (!doingShadows)
	{
		glColor3ub(72, 42, 42);
	}
	gluCylinder(objCylinder, this->wheelRaduis, this->wheelRaduis, this->wheelWidth, 64, 5);


	glPushMatrix();
	glTranslatef(0, 0, this->wheelWidth-0.01);
	static GLUquadricObj *objDisk = gluNewQuadric();
	if (!doingShadows)
	{
		glColor3ub(255, 255, 255);
	}
	gluDisk(objDisk, 0, this->wheelRaduis, 64, 5);
	glRotatef(this->wheelDegree, 0, 0, 1);
	if (!doingShadows)
	{
		glColor3ub(128, 128, 105);
	}
	for (int i = 0; i < 4; i++)
	{
		glRotatef(45, 0, 0, 1);
		glBegin(GL_QUADS);
		glNormal3f(0, 0, 1);
		glVertex3f(-0.1,this->wheelRaduis , 0.01);
		glVertex3f(0.1,this->wheelRaduis , 0.01);
		glVertex3f(0.1,-this->wheelRaduis , 0.01);
		glVertex3f(-0.1,-this->wheelRaduis , 0.01);
		glEnd();
	}
	glPopMatrix();



	glPushMatrix();
	//glTranslatef(0, 0, -this->wheelWidth/2+0.01);

	if (!doingShadows)
	{
		glColor3ub(255, 255, 255);
	}
	gluDisk(objDisk, 0, this->wheelRaduis, 64, 5);
	glRotatef(this->wheelDegree, 0, 0, 1);
	if (!doingShadows)
	{
		glColor3ub(128, 128, 105);
	}
	for (int i = 0; i < 4; i++)
	{
		glRotatef(45, 0, 0, 1);
		glBegin(GL_QUADS);
		glNormal3f(0, 0, 1);
		glVertex3f(-0.1,this->wheelRaduis , -0.01);
		glVertex3f(0.1,this->wheelRaduis , -0.01);
		glVertex3f(0.1,-this->wheelRaduis , -0.01);
		glVertex3f(-0.1,-this->wheelRaduis , -0.01);
		glEnd();
	}
	glPopMatrix();
}

// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();

	// where is the mouse?
	int mx = Fl::event_x();
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPickMatrix((double)mx, (double)(viewport[3] - my),
		5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100, buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
		glLoadName((GLuint)(i + 1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3] - 1;
	}
	else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n", selectedCube);
}

void TrainView::getPos(float t, Pnt3f & pos, int type)
{
	while (t < 0)
	{
		t += this->tw->m_Track.points.size();
	}
	while (t >= this->tw->m_Track.points.size())
	{
		t -= this->tw->m_Track.points.size();
	}
	if (type == 1)
	{
		const ControlPoint* p0, *p1;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[i];
		p1 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];



		Pnt3f qt = (1 - percent) * p0->pos + percent * p1->pos;

		pos = qt;
	}
	else if (type == 2)
	{
		const ControlPoint *p0, *p1, *p2, *p3;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[(i == 0) ? this->m_pTrack->points.size() - 1 : i - 1];
		p1 = &this->m_pTrack->points[(i)];
		p2 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];
		p3 = &this->m_pTrack->points[(i + 2) % this->m_pTrack->points.size()];

		float Gmat[12] =
		{
			p0->pos.x,p0->pos.y,p0->pos.z,
			p1->pos.x,p1->pos.y,p1->pos.z,
			p2->pos.x,p2->pos.y,p2->pos.z,
			p3->pos.x,p3->pos.y,p3->pos.z,
		};
		glm::mat4x3 G = glm::make_mat4x3(Gmat);

		float Tv[4] = { percent*percent*percent,percent*percent,percent,1 };
		glm::vec4 T = glm::make_vec4(Tv);

		glm::vec3 Qt = G * CarrientalM*T;


		pos.x = Qt.x;
		pos.y = Qt.y;
		pos.z = Qt.z;
	}

	else if (type == 3)
	{
		const ControlPoint *p0, *p1, *p2, *p3;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[(i == 0) ? this->m_pTrack->points.size() - 1 : i - 1];
		p1 = &this->m_pTrack->points[(i)];
		p2 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];
		p3 = &this->m_pTrack->points[(i + 2) % this->m_pTrack->points.size()];

		float Gmat[12] =
		{
			p0->pos.x,p0->pos.y,p0->pos.z,
			p1->pos.x,p1->pos.y,p1->pos.z,
			p2->pos.x,p2->pos.y,p2->pos.z,
			p3->pos.x,p3->pos.y,p3->pos.z,
		};
		glm::mat4x3 G = glm::make_mat4x3(Gmat);

		float Tv[4] = { percent*percent*percent,percent*percent,percent,1 };
		glm::vec4 T = glm::make_vec4(Tv);

		glm::vec3 Qt = G * B_SplineM*T;

		pos.x = Qt.x;
		pos.y = Qt.y;
		pos.z = Qt.z;
	}
}

void TrainView::getDir(float t, Pnt3f & dir, int type)
{
	while (t < 0)
	{
		t += this->tw->m_Track.points.size();
	}
	while (t >= this->tw->m_Track.points.size())
	{
		t -= this->tw->m_Track.points.size();
	}
	if (type == 1)
	{
		const ControlPoint* p0, *p1;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[i];
		p1 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];


		dir = (p1->pos - p0->pos);
		dir.normalize();
	}
	else if (type == 2)
	{
		const ControlPoint *p0, *p1, *p2, *p3;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[(i == 0) ? this->m_pTrack->points.size() - 1 : i - 1];
		p1 = &this->m_pTrack->points[(i)];
		p2 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];
		p3 = &this->m_pTrack->points[(i + 2) % this->m_pTrack->points.size()];


		float Gmat[12] =
		{
			p0->pos.x,p0->pos.y,p0->pos.z,
			p1->pos.x,p1->pos.y,p1->pos.z,
			p2->pos.x,p2->pos.y,p2->pos.z,
			p3->pos.x,p3->pos.y,p3->pos.z,
		};
		glm::mat4x3 G = glm::make_mat4x3(Gmat);

		float Tv[4] = { percent*percent*percent,percent*percent,percent,1 };
		glm::vec4 T = glm::make_vec4(Tv);

		float dTv[4] = { 3 * percent*percent,2 * percent,1,0 };
		glm::vec4 dT = glm::make_vec4(dTv);
		glm::vec3 dQt = G * CarrientalM*dT;
		dir.x = dQt.x;
		dir.y = dQt.y;
		dir.z = dQt.z;

		dir.normalize();
	}

	else if (type == 3)
	{
		const ControlPoint *p0, *p1, *p2, *p3;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[(i == 0) ? this->m_pTrack->points.size() - 1 : i - 1];
		p1 = &this->m_pTrack->points[(i)];
		p2 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];
		p3 = &this->m_pTrack->points[(i + 2) % this->m_pTrack->points.size()];

		float Gmat[12] =
		{
			p0->pos.x,p0->pos.y,p0->pos.z,
			p1->pos.x,p1->pos.y,p1->pos.z,
			p2->pos.x,p2->pos.y,p2->pos.z,
			p3->pos.x,p3->pos.y,p3->pos.z,
		};
		glm::mat4x3 G = glm::make_mat4x3(Gmat);


		float Tv[4] = { percent*percent*percent,percent*percent,percent,1 };
		glm::vec4 T = glm::make_vec4(Tv);

		float dTv[4] = { 3 * percent*percent,2 * percent,1,0 };
		glm::vec4 dT = glm::make_vec4(dTv);
		glm::vec3 dQt = G * B_SplineM*dT;
		dir.x = dQt.x;
		dir.y = dQt.y;
		dir.z = dQt.z;

		dir.normalize();
	}
}

void TrainView::getOrient(float t, Pnt3f& up, int type)
{
	while (t < 0)
	{
		t += this->tw->m_Track.points.size();
	}
	while (t >= this->tw->m_Track.points.size())
	{
		t -= this->tw->m_Track.points.size();
	}
	if (type == 1)
	{
		const ControlPoint* p0, *p1;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[i];
		p1 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];

		Pnt3f orient_t = (1 - percent) * p0->orient + percent * p1->orient;
		orient_t.normalize();

		up = orient_t;
		up.normalize();

	}
	else if (type == 2)
	{
		const ControlPoint *p0, *p1, *p2, *p3;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[(i == 0) ? this->m_pTrack->points.size() - 1 : i - 1];
		p1 = &this->m_pTrack->points[(i)];
		p2 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];
		p3 = &this->m_pTrack->points[(i + 2) % this->m_pTrack->points.size()];


		float Tv[4] = { percent*percent*percent,percent*percent,percent,1 };
		glm::vec4 T = glm::make_vec4(Tv);


		float Omat[12] =
		{
			p0->orient.x,p0->orient.y,p0->orient.z,
			p1->orient.x,p1->orient.y,p1->orient.z,
			p2->orient.x,p2->orient.y,p2->orient.z,
			p3->orient.x,p3->orient.y,p3->orient.z,
		};
		glm::mat4x3 O = glm::make_mat4x3(Omat);
		glm::vec3 Ot = O * CarrientalM*T;

		up.x = Ot.x;
		up.y = Ot.y;
		up.z = Ot.z;

		up.normalize();
	}

	else if (type == 3)
	{
		const ControlPoint *p0, *p1, *p2, *p3;
		int i = floor(t);
		float percent = t - i;
		p0 = &this->m_pTrack->points[(i == 0) ? this->m_pTrack->points.size() - 1 : i - 1];
		p1 = &this->m_pTrack->points[(i)];
		p2 = &this->m_pTrack->points[(i + 1) % this->m_pTrack->points.size()];
		p3 = &this->m_pTrack->points[(i + 2) % this->m_pTrack->points.size()];


		float Tv[4] = { percent*percent*percent,percent*percent,percent,1 };
		glm::vec4 T = glm::make_vec4(Tv);

		float Omat[12] =
		{
			p0->orient.x,p0->orient.y,p0->orient.z,
			p1->orient.x,p1->orient.y,p1->orient.z,
			p2->orient.x,p2->orient.y,p2->orient.z,
			p3->orient.x,p3->orient.y,p3->orient.z,
		};
		glm::mat4x3 O = glm::make_mat4x3(Omat);
		glm::vec3 Ot = O * B_SplineM*T;

		up.x = Ot.x;
		up.y = Ot.y;
		up.z = Ot.z;

		up.normalize();
	}
}
