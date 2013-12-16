/****************************
Copyright (c) 2013 Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
****************************/

#ifndef GL_PLATFORM_H
#define GL_PLATFORM_H

#if !defined(USE_SDL2)
#define USE_SDL1 1
#endif

#ifdef __IPHONEOS__
#define USING_GLES 1
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#elif defined(ANDROID)
#define USING_GLES 1
#include <GLES/gl.h>
#include <GLES/glext.h>
#elif defined(FLASCC)
#define USING_GLES 1
#include <GLES/gl.h>
#elif RASPBERRYPI
#define USING_GLES 1
#define USING_GLDESKTOP 0
#define REQUIRE_EGL 1
#include <GLES/gl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#elif defined(USE_SDL1)
#define USING_GLDESKTOP 1
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#elif defined(USE_SDL2)
#define USING_GLDESKTOP 1
#include <SDL.h>
#include <SDL_opengl.h>
#endif





#endif // GL_PLATFORM_H