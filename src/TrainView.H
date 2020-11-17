/************************************************************************
	 File:        TrainView.H

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

#pragma once

// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;


//#######################################################################
// TODO
// You might change the TrainView in order to add different objects to
// be drawn, or adjust the lighting, or ...
//#######################################################################


// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Gl_Window.h>
#pragma warning(pop)

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"

#include "Utilities/Pnt3f.H"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
class TrainView : public Fl_Gl_Window
{
public:
	// note that we keep the "standard widget" constructor arguments
	TrainView(int x, int y, int w, int h, const char* l = 0);

	// overrides of important window things
	virtual int handle(int);
	virtual void draw();

	// all of the actual drawing happens in this routine
	// it has to be encapsulated, since we draw differently if
	// we're drawing shadows (no colors, for example)
	void drawStuff(bool doingShadows = false);

	void drawTrack(Pnt3f pv, Pnt3f cv, Pnt3f cross_t, bool doingShadows);
	void drawBar(Pnt3f pos, Pnt3f dir, Pnt3f up, bool doingShadows);
	void drawCarts(float t, bool doingShadows);
	void drawWheel(bool doingShadows);

	// setup the projection - assuming that the projection stack has been
	// cleared for you
	void setProjection();

	// Reset the Arc ball control
	void resetArcball();

	// pick a point (for when the mouse goes down)
	void doPick();

	void getPos( float t, Pnt3f& pos, int type);
	void getDir( float t, Pnt3f& dir, int type);
	void getOrient( float t, Pnt3f& up, int type);

public:
	ArcBallCam		arcball;			// keep an ArcBall for the UI
	int				selectedCube;  // simple - just remember which cube is selected

	TrainWindow*	tw;				// The parent of this display window
	CTrack*			m_pTrack;		// The track of the entire scene
	struct BarNeeds
	{
		Pnt3f pos;
		Pnt3f dir;
		Pnt3f up;
	};
	struct TrackNeeds
	{
		Pnt3f pv;
		Pnt3f cv;
		Pnt3f cross_t;
	};
	std::vector<BarNeeds> barDrawList;
	std::vector<TrackNeeds> trackDrawList;

	int cartsCount = 5;
	const float cartsSpacing = 17;

	const float defaultSpeed = 75;
	const float maxSpeed = defaultSpeed*4;	
	const float minSpeed = defaultSpeed/2;
	float currTrainSpeed = defaultSpeed;
	const float gravityFactor = 9.8/2;
	

	const float trainWidth = 4.5;
	const float trainHeight = 6;
	const float trainLength = 15;
	const float wheelRaduis = 1;
	const float wheelWidth = .5;
	float wheelDegree = 0.0f;


	const int DIVIDE_LINE = 1000;
	const float barSpacing = 7.5;

	const float B_SplineMmat[16] =
	{
		-1 / 6.0,3 / 6.0,-3 / 6.0,1 / 6.0,
		3 / 6.0,-6 / 6.0,3 / 6.0,0 / 6.0,
		-3 / 6.0,0 / 6.0,3 / 6.0,0 / 6.0,
		1 / 6.0,4 / 6.0,1 / 6.0,0 / 6.0
	};
	const glm::mat4  B_SplineM = glm::make_mat4(B_SplineMmat);
	const float CarrientalMmat[16] =
	{
		-1 / 2.0 ,3 / 2.0 ,-3 / 2.0  ,1 / 2.0,
		2 / 2.0  ,-5 / 2.0,4 / 2.0   ,-1 / 2.0,
		-1 / 2.0 ,0 / 2.0 ,1 / 2.0   ,0 / 2.0,
		0 / 2.0  ,2 / 2.0 ,0 / 2.0   ,0 / 2.0
	};
	const glm::mat4  CarrientalM = glm::make_mat4(CarrientalMmat);
};