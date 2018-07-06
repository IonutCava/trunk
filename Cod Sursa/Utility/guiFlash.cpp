//-----------------------------------------------------------------------------
// (c) Thomas "Man of Ice" Lund and Brian "Bzztbomb" Richardson, 2005
// Free to use for all Torque SDK owners
// 
// Unreleased changes
//	-none
//
// Version 1.5 - 2005-09-05
//	- commented out the gameswf::clear() to get rid of exit crash bug
//  - updated with CVS from 5th of September 2005
//  - added anti alias switch
//
// Version 1.4 - 2005-04-23
//  - resizing the component now reinitializes the movie to keep it from going blank
//	- added gotoFrame method that moves to the numbered frame
//	- added gotoNamedFrame method that moves to the named frame
//	- added callMethod method to call functionality inside flash movies
//	- reorganized files, so its easier to merge gameswf and base files
//	- Updated GameSWF files with CVS version of GameSWF from 23rd April 2005
//
// Version 1.3 - 2005-03-02
//  - Fixed bug with mouse press being handled wrongly, resulting in having to double click on buttons
//	- Updated GameSWF files with CVS version of GameSWF from 2nd March 2005
//
// Version 1.2 - 2005-02-08
//  - Fix to the command callback. Should work now
//  - Added stopMovie command
//  - Added restartMovie command
//	- Added stubbed out sound support
//	- Added setting/getting flash variables
//  - Fixed bug with mouse coordinates when rescaling the gui
//
// Version 1.1 - 2005-02-05
//  - Little restructuring of file to be more readable
//  - Implemented mErrorDisplayed variable, so errors are only shown once instead of spamming the console
//  - Extended the loadMovie() method to take optional looping and alpha values to save some typing
//  - Implemented sending keyboard input to the flash movie
//
// Version 1.0 - 2005-02-04
//  - First public release
//
// Known errors and missing functionality:
// - sound isnt coded yet
// - variables only support strings
// 
//-----------------------------------------------------------------------------

#include "GUI/GUI.h"
#include "gameswf/gameswf.h"
#include "gameswf/base/utility.h"
#include "gameswf/base/container.h"
#include "gameswf/base/tu_file.h"
#include "gameswf/base/tu_types.h"

class GuiFlash : public GuiControl
{
   typedef GuiControl Parent;

	bool	mMovieInitialized;
	bool	mPlaying;
	char*	mFlashMovieName;
	bool	mLoopMovie;
	F32		mBackgroundAlpha;
	bool	mErrorDisplayed;

	// Movie vars
	int	movie_width;
	int	movie_height;

	// Mouse vars
	int	mouse_x;
	int	mouse_y;
	int	mouse_buttons;

	F32 mouse_x_scale;
	F32 mouse_y_scale;

	gameswf::sound_handler*		mSoundHandler;
	gameswf::render_handler*	mRenderHandler;
	gameswf::movie_definition*	mMovieDefinition;
	gameswf::movie_interface*	mMovieInterface;

	// Store variables we've returned
	vector<char*> mFlashVars;
	
	U32	last_ticks;

	void initializeMovie();
	gameswf::key::code translateKeymap(U32 tgeKey);
	void computeMouseScale(const vec2& extent);

public:
   GuiFlash();
   ~GuiFlash();

	// Flash Stuff
	void loadMovie(const char* name);
	void playMovie();
	void stopMovie();
	void restartMovie();
	void setBackgroundAlpha(F32 alpha);
	void setLooping(bool loop);
	void gotoFrame(I32 frame);
	void gotoLabeledFrame(const char* frame);
	void setAntialiased(bool enabled);

	// Keyboard events
	/*
   bool onKeyUp(const GuiEvent &event);
   bool onKeyDown(const GuiEvent &event);

	// Mouse events
   void onMouseMove(const GuiEvent &event);
   void onMouseUp(const GuiEvent &event);
   void onMouseDown(const GuiEvent &event);
   void onRightMouseUp(const GuiEvent &event);
   void onRightMouseDown(const GuiEvent &event);
*/
   	// Flash variable support
	void setFlashVariable(const char* pathToVariable, const char* newValue);
	const char* getFlashVariable(const char* pathToVariable);

	// Calling flash methods
	const char* callMethod(const char* methodName, const char* argFormat, ...);

	void onRender( vec2, const vec4 &);
	void resize(const vec2 &newPosition, const vec2 &newExtent);


   static void initPersistFields();
};

/*
//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------

// Error callback for handling gameswf messages.
static void	log_callback(bool error, const char* message)
{
	Con::errorf("GameSWF error: %s", message);
}


// Callback function.  This opens files for the gameswf library.
static tu_file*	file_opener(const char* url)
{
	return new tu_file(url, "rb");
}

// Triggered by gameswf when an actionscript file calls fsCommand
void guiFlashFsCommand(gameswf::movie_interface* movie, const char* command, const char* arg)
{
	Con::evaluate(command);
}

//-----------------------------------------------------------------------------
// The gui component code
//-----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT( GuiFlash );

GuiFlash::GuiFlash()
{
	mMovieInitialized = false;
	mLoopMovie = false;
	mFlashMovieName = "test.swf\0";
	mPlaying = false;
	mBackgroundAlpha = 1.0f;
	mErrorDisplayed = false;

	mSoundHandler = NULL;
	mRenderHandler = NULL;
	mMovieDefinition = NULL;
	mMovieInterface = NULL;

	mouse_x = 0;
	mouse_y = 0;
	mouse_buttons = 0;

	movie_width = 0;
	movie_height = 0;

	last_ticks = 0;
}

GuiFlash::~GuiFlash()
{
	if (mMovieDefinition)
	{
		mMovieDefinition->drop_ref();
	}
	if (mMovieInterface)
	{
		mMovieInterface->drop_ref();
	}
	delete mSoundHandler;
	delete mRenderHandler;

	// Clean up our strings
	for (S32 i = 0; i < mFlashVars.size(); i++)
	{
		delete[] mFlashVars[i];
	}

	// Clean up gameswf as much as possible, so valgrind will help find actual leaks.
	//gameswf::clear();
}

void GuiFlash::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Flash");
   addField( "movieName", TypeCaseString, Offset( mFlashMovieName, GuiFlash ) );
   addField( "backgroundAlpha", TypeF32, Offset( mBackgroundAlpha, GuiFlash ) );
   addField( "loopMovie", TypeBool, Offset( mLoopMovie, GuiFlash ) );
   endGroup("Flash");
}

void GuiFlash::initializeMovie()
{
	gameswf::register_file_opener_callback(file_opener);
	gameswf::register_log_callback(log_callback);
	gameswf::register_fscommand_callback(guiFlashFsCommand);

	mSoundHandler = gameswf::create_sound_handler_tge();
	gameswf::set_sound_handler(mSoundHandler);
	mRenderHandler = gameswf::create_render_handler_ogl();
	gameswf::set_render_handler(mRenderHandler); 

	// Get info about the width & height of the movie.
	int	movie_version = 0;
	float	movie_fps = 30.0f;
	gameswf::get_movie_info(mFlashMovieName, &movie_version, &movie_width, &movie_height, &movie_fps, NULL, NULL);
	if (movie_version == 0)
	{
		Con::errorf("error: can't get info about %s\n", mFlashMovieName);
		mMovieInitialized = false;
		mErrorDisplayed = true;
		return;
	}
	Con::printf("Flash file loaded successfully\nName %s\nVersion %i\nWidth %i\nHeight %i\nFPS %i", mFlashMovieName, movie_version, movie_width, movie_height, movie_fps);

	computeMouseScale(mBounds.extent);

	// Load the actual movie.
	mMovieDefinition = gameswf::create_movie(mFlashMovieName);
	if (mMovieDefinition == NULL)
	{
		Con::errorf("error: can't create a movie from '%s'\n", mFlashMovieName);
		mMovieInitialized = false;
		mErrorDisplayed = true;
		return;
	}
	mMovieInterface = mMovieDefinition->create_instance();
	if (mMovieInterface == NULL)
	{
		fprintf(stderr, "error: can't create movie instance\n");
		mMovieInitialized = false;
		mErrorDisplayed = true;
		return;
	}

	last_ticks = Platform::getRealMilliseconds();

	mMovieInitialized = true;	
}

// Called on resize
void GuiFlash::resize(const Point2I &newPosition, const Point2I &newExtent)
{
	Parent::resize(newPosition, newExtent);
	initializeMovie();
}

void GuiFlash::computeMouseScale(const Point2I& extent)
{
	// Figure out x and y scale for mouse coords
	mouse_x_scale = (F32) movie_width / (F32) extent.x;
	mouse_y_scale = (F32) movie_height / (F32) extent.y;
}

//-----------------------------------------------------------------------------
// Gui onRender method.
//-----------------------------------------------------------------------------
void GuiFlash::onRender(Point2I offset, const RectI &updateRect)
{
	if (mMovieInitialized == false && mErrorDisplayed == false)
	{
		initializeMovie();
	}

	// Play movie
	if (mMovieInitialized == true && mPlaying == true)
	{
		U32 ticks = Platform::getRealMilliseconds();
		int	delta_ticks = ticks - last_ticks;
		float	delta_t = delta_ticks / 1000.f;
		last_ticks = ticks;

		glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-1, 0, 1, -1, 1, 1);
        glMatrixMode(GL_MODELVIEW);

        mMovieInterface->set_display_viewport(offset.x, offset.y, updateRect.extent.x, updateRect.extent.y);
        mMovieInterface->set_background_alpha(mBackgroundAlpha);
		mMovieInterface->notify_mouse_state(mouse_x, mouse_y, mouse_buttons);
        mMovieInterface->advance(delta_t);               
		mMovieInterface->display();       

        // Restore state that gameswf kills
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW); 

		// See if we should exit.
		if (mLoopMovie == false && (mMovieInterface->get_current_frame() + 1 == mMovieDefinition->get_frame_count() 
									|| mMovieInterface->get_play_state() == gameswf::movie_interface::STOP))
		{
			// We're reached the end of the movie; exit.
			mPlaying = false;
			Con::errorf("Flash movie done playing");
		}

		dglSetCanonicalState();

	}
	// The movie has been loaded, but is either finished or not started yet. Display first/last frame
	if (mMovieInitialized == true && mPlaying == false)
	{
		glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-1, 0, 1, -1, 1, 1);
        glMatrixMode(GL_MODELVIEW);

        mMovieInterface->set_display_viewport(offset.x, offset.y, updateRect.extent.x, updateRect.extent.y);
        mMovieInterface->set_background_alpha(mBackgroundAlpha);
		mMovieInterface->display();       

        // Restore state that gameswf kills
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW); 

		dglSetCanonicalState();

	}
}

//-----------------------------------------------------------------------------
// Keyboard events
//-----------------------------------------------------------------------------

bool GuiFlash::onKeyDown(const GuiEvent &event)
{
	gameswf::key::code	c(gameswf::key::INVALID);

	c = translateKeymap(event.keyCode);

	if (c != gameswf::key::INVALID)
	{
		gameswf::notify_key_event(c, true);
	}
	//pass the event to the parent
	GuiControl* parent = getParent();
	if (parent)
	{
		return parent->onKeyDown(event);
	}
	else
	{
		return false;
	}
}

bool GuiFlash::onKeyUp(const GuiEvent &event)
{
	gameswf::key::code	c(gameswf::key::INVALID);

	c = translateKeymap(event.keyCode);

	if (c != gameswf::key::INVALID)
	{
		gameswf::notify_key_event(c, false);
	}
	//pass the event to the parent
	GuiControl* parent = getParent();
	if (parent)
	{
		return parent->onKeyDown(event);
	}
	else
	{
		return false;
	}
}

gameswf::key::code GuiFlash::translateKeymap(U32 tgeKey)
{
	// Return keys we can group in ranges
	if (tgeKey >= KEY_A && tgeKey <= KEY_Z)
	{
		return (gameswf::key::code) ((tgeKey - KEY_A) + gameswf::key::A);
	}
	else if (tgeKey >= KEY_F1 && tgeKey <= KEY_F15)
	{
		return (gameswf::key::code) ((tgeKey - KEY_F1) + gameswf::key::F1);
	}
	else if (tgeKey >= KEY_0 && tgeKey <= KEY_9)
	{
		return (gameswf::key::code) ((tgeKey - KEY_0) + gameswf::key::_0);
	}
	else if (tgeKey >= KEY_NUMPAD0 && tgeKey <= KEY_NUMPAD9)
	{
		return (gameswf::key::code) ((tgeKey - KEY_NUMPAD0) + gameswf::key::KP_0);
	}
	// Switch on individual keys
	switch(tgeKey)
	{
	case KEY_BACKSLASH:
		return gameswf::key::BACKSLASH;
	case KEY_BACKSPACE:
		return gameswf::key::BACKSPACE;
	case KEY_NUMPADENTER:
		return gameswf::key::KP_ENTER;
	case KEY_TAB:
		return gameswf::key::TAB;
	case KEY_RETURN:
		return gameswf::key::ENTER;
	case KEY_SHIFT:
			return gameswf::key::SHIFT;
	case KEY_CONTROL:
			return gameswf::key::CONTROL;
	case KEY_ALT:
			return gameswf::key::ALT;
	case KEY_CAPSLOCK:
			return gameswf::key::CAPSLOCK;
	case KEY_ESCAPE:
			return gameswf::key::ESCAPE;
	case KEY_SPACE:
			return gameswf::key::SPACE;
	case KEY_PAGE_DOWN:
			return gameswf::key::PGDN;
	case KEY_PAGE_UP:
			return gameswf::key::PGUP;
	case KEY_END:
			return gameswf::key::END;
	case KEY_HOME:
			return gameswf::key::HOME;
	case KEY_LEFT:
			return gameswf::key::LEFT;
	case KEY_UP:
			return gameswf::key::UP;
	case KEY_RIGHT:
			return gameswf::key::RIGHT;
	case KEY_DOWN:
			return gameswf::key::DOWN;
	case KEY_INSERT:
			return gameswf::key::INSERT;
	case KEY_DELETE:
			return gameswf::key::DELETEKEY;
	case KEY_HELP:
			return gameswf::key::HELP;
	case KEY_NUMLOCK:
			return gameswf::key::NUM_LOCK;
	case KEY_SEMICOLON:
			return gameswf::key::SEMICOLON;
	case KEY_EQUALS:
			return gameswf::key::EQUALS;
	case KEY_MINUS:
			return gameswf::key::MINUS;
	case KEY_SLASH:
			return gameswf::key::SLASH;
	case KEY_LBRACKET:
			return gameswf::key::LEFT_BRACKET;
	case KEY_RBRACKET:
			return gameswf::key::RIGHT_BRACKET;
	case KEY_APOSTROPHE:
			return gameswf::key::QUOTE;
	}

	// Default to invalid key
	return gameswf::key::INVALID;

	// Keys in GameSWF without mapping in TGE:
	//	return gameswf::key::KP_MULTIPLY;
	//	return gameswf::key::KP_ADD;
	//	return gameswf::key::KP_SUBTRACT;
	//	return gameswf::key::KP_DECIMAL;
	//	return gameswf::key::KP_DIVIDE;
	//	return gameswf::key::CLEAR;
	//	return gameswf::key::BACKTICK;
	
}

//-----------------------------------------------------------------------------
// Mouse events
//-----------------------------------------------------------------------------
void GuiFlash::onMouseMove(const GuiEvent &event)
{	
	Point2I localPoint = globalToLocalCoord(event.mousePoint);
	mouse_x = localPoint.x * mouse_x_scale;
	mouse_y = localPoint.y * mouse_y_scale;	
}

void GuiFlash::onMouseUp(const GuiEvent &event)
{	
	mouse_buttons &= ~1;		// 1 = left mouse button
}

void GuiFlash::onMouseDown(const GuiEvent &event)
{
	mouse_buttons |= 1;		// 1 = left mouse button
}

void GuiFlash::onRightMouseUp(const GuiEvent &event)
{	
	mouse_buttons &= ~2;		// 2 = right mouse button
}

void GuiFlash::onRightMouseDown(const GuiEvent &event)
{
	mouse_buttons |= 2;		// 2 = right mouse button
}

//-----------------------------------------------------------------------------
// Getters/setters
//-----------------------------------------------------------------------------

void GuiFlash::loadMovie(const char* name)
{
	mMovieInitialized = false;
	mErrorDisplayed = false;
	mFlashMovieName = const_cast<char*>(name);
	initializeMovie();
}

void GuiFlash::playMovie()
{
	mPlaying = true;
	if (mMovieInterface)
	{
		mMovieInterface->set_play_state(gameswf::movie_interface::PLAY);
	}
}

void GuiFlash::stopMovie()
{
	mPlaying = false;
	if (mMovieInterface)
	{
		mMovieInterface->set_play_state(gameswf::movie_interface::STOP);
	}
}

void GuiFlash::restartMovie()
{
	mPlaying = false;
	mMovieInitialized = false;
	mErrorDisplayed = false;
	initializeMovie();
	mPlaying = true;
}

void GuiFlash::gotoFrame(S32 frame)
{
	if (mMovieInterface)
	{
		mMovieInterface->goto_frame(frame);
	}
}

void GuiFlash::gotoLabeledFrame(const char* frame)
{
	if (mMovieInterface)
	{
		mMovieInterface->goto_labeled_frame(frame);
	}
}

void GuiFlash::setBackgroundAlpha(F32 alpha)
{
	alpha = mClampF(alpha, 0.0f, 1.0f);
	mBackgroundAlpha = alpha;
}

void GuiFlash::setLooping(bool loop)
{
	mLoopMovie = loop;
}

// Set's a variable in flash, example _root.myVar
void GuiFlash::setFlashVariable(const char* pathToVariable, const char* newValue)
{
	mMovieInterface->set_variable(pathToVariable, newValue);
}

// Gets a flash variable, the buffer returned is valid until the control
// is deleted.  
const char* GuiFlash::getFlashVariable(const char* pathToVariable)
{
	char* buffer = new char[256];
	const char* tempResult = mMovieInterface->get_variable(pathToVariable);
	dStrncpy(buffer, tempResult, 256);	
	mFlashVars.push_back(buffer);
	return buffer;
}

// Call method inside flash movie and return result as string
const char* GuiFlash::callMethod(const char* methodName, const char* argFormat, ...)
{
	char* buffer = new char[256];

	va_list	argList;
	va_start(argList, argFormat);
	const char* tempResult = mMovieInterface->call_method_args(methodName, argFormat, argList);
	va_end(argList);

	dStrncpy(buffer, tempResult, 256);
	return buffer;
}

// Sets the anti alias flag
void GuiFlash::setAntialiased(bool enabled)
{
	mRenderHandler->set_antialiased(enabled);
}

//-----------------------------------------------------------------------------
// Console methods
//-----------------------------------------------------------------------------

ConsoleMethod( GuiFlash, loadMovie, void, 3, 5, "(string name, bool looping, F32 alpha)"
              "Filename of flash movie, optional looping, optional alpha of background")
{
	object->loadMovie(argv[2]);

	if (argv[3])
	{
		object->setLooping(dAtob(argv[3]));
	}

	if (argv[4])
	{
		F32 alpha;
		dSscanf(argv[4], "%f", &alpha);
		object->setBackgroundAlpha(alpha);
	}
}

ConsoleMethod( GuiFlash, playMovie, void, 2, 2, "" "")
{
	object->playMovie();
}

ConsoleMethod( GuiFlash, stopMovie, void, 2, 2, "" "")
{
	object->stopMovie();
}

ConsoleMethod( GuiFlash, restartMovie, void, 2, 2, "" "")
{
	object->restartMovie();
}

ConsoleMethod( GuiFlash, gotoFrame, void, 3, 3, "(S32 frame)" "Frame number to go to")
{
	object->gotoFrame(dAtoi(argv[2]));
}

ConsoleMethod( GuiFlash, gotoLabeledFrame, void, 3, 3, "(string frame)" "Frame label to go to")
{
	object->gotoLabeledFrame(argv[2]);
}

ConsoleMethod( GuiFlash, setBackgroundAlpha, void, 3, 3, "(F32 alpha)"
              "Alpha level of background between 0 and 1")
{
	F32 alpha;
	dSscanf(argv[2], "%f", &alpha);
	object->setBackgroundAlpha(alpha);
}

ConsoleMethod( GuiFlash, setLooping, void, 3, 3, "(bool loop)"
              "Should the movie loop?")
{
	object->setLooping(dAtob(argv[2]));
}

ConsoleMethod( GuiFlash, setFlashVariable, void, 4, 4, "(string pathToVariable, string newValue)"
              "Sets a flash variable.")
{
	object->setFlashVariable(argv[2], argv[3]);	
}

ConsoleMethod( GuiFlash, getFlashVariable, const char*, 3, 3, "(string pathToVariable)"
              "Gets a flash variable.")
{	
	return object->getFlashVariable(argv[2]);
}

ConsoleMethod( GuiFlash, callMethod, const char*, 4, 14, "(string methodName, string methodFormat, string methodArgs)"
              "Calls a flash method and returns result. Takes up to 10 arguments")
{	
	return object->callMethod(argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13]);
}

ConsoleMethod( GuiFlash, setAntialiased, void, 3, 3, "(bool enabled)"
              "Enable/disable anti aliased rendering")
{
	object->setAntialiased(dAtob(argv[2]));
}

*/