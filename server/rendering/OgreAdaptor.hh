/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003  
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* Desc: Middleman between OGRE and Gazebo
 * Author: Nate Koenig
 * Date: 13 Feb 2006
 * CVS: $Id: OgreAdaptor.hh,v 1.1.2.1 2006/12/16 22:41:17 natepak Exp $
 */

#ifndef OGREADAPTOR
#define OGREADAPTOR

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace Ogre
{
  class Root;
  class SceneManager;
  class RenderWindow;
  class Viewport;
  class InputReader;
  class Window;
  class Camera;
  class SceneNode;
  class LogManager;
  class Overlay;
  class OverlayContainer;
}

namespace gazebo
{

class XMLConfigNode;
class OgreFrameListener;

class OgreAdaptor
{
  /// Constructor
  private: OgreAdaptor();

  /// Destructor
  public: virtual ~OgreAdaptor();

  public: static OgreAdaptor *Instance();

  /// Default initialization. Let OGRE create the window and rendering context
  public: void Init(XMLConfigNode *node);

  /// Initialize Ogre Rendering engine
  public: void Init(Display *display, XVisualInfo *visual, Window windowId, int width, int height);

  /// Render a single frame
  public: int Render();

  private: void LoadPlugins(XMLConfigNode *node);
  private: void SetupResources(XMLConfigNode *node);
  private: void SetupRenderSystem(bool create);
  private: void CreateWindow(int width, int height);

  public: Ogre::Root *root;
  public: Ogre::SceneManager *sceneMgr;
  public: Ogre::RenderWindow *window;
  public: Ogre::Camera *camera;
  public: Ogre::Viewport *viewport;
  public: Ogre::InputReader *inputDevice;

  private: Ogre::LogManager *logManager;

  // Our custom frame listener
  private: OgreFrameListener *frameListener;

  /// Render context info
  private: Display *display;
  private: XVisualInfo *visual;
  private: Window windowId;

  private: static OgreAdaptor *myself;
};

class OgreGLXWindowInterface
{
  public: virtual ~OgreGLXWindowInterface() = 0;

  // Call this with true when the window is mapped/visible, false when the window is unmapped/invisible
  public: virtual void exposed(bool active) = 0;

  // Call this to notify the window was resized
  public: virtual void resized(size_t width, size_t height) = 0;
};

}
#endif
