/****************************
Copyright (c) 2006-2012 Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
****************************/

/* Required Headers */
#include "luxengine.h"
#include "map_object.h"
#include "mokoi_game.h"
#include "elix_png.hpp"
#include "game_config.h"
#include "core.h"

#include "display_types.h"
#include "graphics_opengl.h"
#include "graphics_system.h"
#include "gles/gles.hpp"
#include "gles/gles_display.hpp"
#include "gles/gl_camera.hpp"
#include "gles/texture.inc.hpp"

#include "platform_functions.h"


GraphicSystem GraphicsOpenGL = {
	&Lux_OGL_Init,
	&Lux_OGL_Destory,
	&Lux_OGL_Display2Screen,
	&Lux_GLES_TextSprites,
	&Lux_OGL_Update,
	&Lux_OGL_Show,

	&Lux_OGL_SetRotation,
	&Lux_OGL_BackgroundObject,
	&Lux_OGL_SetFullscreen,
	&Lux_OGL_Resize,
	&Lux_OGL_DisplayPointer,

	&Lux_OGL_RefreshSpriteSheet,
	&Lux_OGL_LoadSpriteSheet,
	&Lux_OGL_LoadSpriteSheetImage,
	&Lux_OGL_FreeSpriteSheet,
	NULL,
	&Lux_OGL_FreeSprite,
	&Lux_OGL_PNGtoSprite,

	&Lux_GLES_DrawSprite,
	&Lux_GLES_DrawRect,
	&Lux_GLES_DrawCircle,
	&Lux_GLES_DrawPolygon,
	&Lux_GLES_DrawLine,
	&Lux_GLES_DrawText,
	&Lux_OGL_DrawMessage,

	&Lux_OGL_CacheDisplay,
	&Lux_OGL_DrawCacheDisplay
};

/* Shared */
extern Uint32 native_render_flags;
extern Uint32 native_window_flags;
extern SDL_Window * native_window;
extern SDL_Renderer * native_renderer;
extern std::string native_window_title;
extern SDL_Rect native_graphics_dimension;


SDL_Window * debug_window = NULL;
SDL_Renderer * debug_renderer = NULL;

extern uint32_t sdlgraphics_fps, sdlgraphics_fpstime;
/* Global Variables */
LuxColour gles_graphics_colour = { 0, 0, 0, 255 };

SDL_Rect native_graphics_viewpoint;
SDL_GLContext native_context;
float opengl_graphic_ratio_width = 1.00;
float opengl_graphic_ratio_height = 1.00;
float opengl_graphic_ratio = 1.33;
LuxPolygon * opengl_graphic_cursor = NULL;
SDL_Rect native_screen_position;
bool native_screen_stretching = false;
bool native_screen_match = false;

Texture fbo_textures[8] = {0};
GLuint fbo_frame, fbo_depthBuffer;
bool fbo_supported = false;

/* Local Function */
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;


void opengl_graphic_create_fbotexture( Texture & fbo )
{
	fbo.w = fbo.tw = (native_graphics_dimension.w);
	fbo.h = fbo.th = (native_graphics_dimension.h);

	fbo.pot = true;

	glGenTextures(1, &fbo.pointer);

	glBindTexture(GL_TEXTURE_2D, fbo.pointer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbo.tw, fbo.th, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.pointer, 0);

	fbo.loaded = true;
}

/* Global Function */

/* Lux_OGL_Init
 * Init video mode
 @ width:
 @ height:
 @ bpp:
 @ fullscreen:
 - Returns true if successfull
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_Init( std::string title,  uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen )
{
	int window_width = width;
	int window_height = height;

	#if DISPLAYMODE_NATIVE
	if ( lux::config->GetString("display.mode") != "OpenGL" )
		return false;
	#endif

	/* Set Init Flags */
	native_window_flags = SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL;

	if ( fullscreen )
		native_window_flags |= SDL_WINDOW_FULLSCREEN;

	native_render_flags = SDL_RENDERER_ACCELERATED;
	native_window_title = title  + " (OpenGL Rendering)";

	//native_screen_match = lux::config->GetBoolean("screen.matchdisplay");

	if ( width > height )
	{
		SDL_SetHintWithPriority( SDL_HINT_ORIENTATIONS, "LandscapeLeft", SDL_HINT_OVERRIDE );
	}
	else
	{
		SDL_SetHintWithPriority( SDL_HINT_ORIENTATIONS, "Portrait", SDL_HINT_OVERRIDE );
	}

	native_window = lux::core->GetWindow();

	SDL_SetWindowSize(native_window, width, height );
	SDL_GetWindowSize(native_window, &window_width, &window_height);


	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_SetHintWithPriority( SDL_HINT_RENDER_OPENGL_SHADERS, "1", SDL_HINT_OVERRIDE );


	native_context = SDL_GL_CreateContext(native_window);
	if ( !native_context )
	{
		lux::core->SystemMessage(SYSTEM_MESSAGE_INFO, __FILE__, __LINE__) << " Couldn't create Renderer. " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_GL_MakeCurrent(native_window, native_context);

	lux::core->SystemMessage(SYSTEM_MESSAGE_INFO) << __FILE__ << ": | Vendor: " << glGetString(GL_VENDOR) << std::endl;
	lux::core->SystemMessage(SYSTEM_MESSAGE_INFO) << __FILE__ << ": | Renderer: " << glGetString(GL_RENDERER) << std::endl;
	lux::core->SystemMessage(SYSTEM_MESSAGE_INFO) << __FILE__ << ": | Version: " << glGetString(GL_VERSION) << std::endl;
	lux::core->SystemMessage(SYSTEM_MESSAGE_INFO) << __FILE__ << ": | GL_ARB_texture_non_power_of_two: " << SDL_GL_ExtensionSupported((char *)"GL_ARB_texture_non_power_of_two") << std::endl;

	native_graphics_dimension.w = width;
	native_graphics_dimension.h = height;
	opengl_graphic_ratio = (float)height / (float)width;

	Lux_OGL_Resize( window_width, window_height );

	gles::setOrtho( 0.0f, (float)native_graphics_dimension.w, (float)native_graphics_dimension.h, 0.0f, -10.0f, 10.0f );

	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST );
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	OpenGLShader::Check();
	Lux_GLES_LoadFont();

	glEnable(GL_TEXTURE_2D);
	glClearColor((float)gles_graphics_colour.r / 255.0f, (float)gles_graphics_colour.g / 255.0f, (float)gles_graphics_colour.b / 255.0f, 1.0f);


	opengl_graphic_cursor = new LuxPolygon(3);

	opengl_graphic_cursor->SetPoint(0, 0, 0);
	opengl_graphic_cursor->SetPoint(1, 8, 8);
	opengl_graphic_cursor->SetPoint(2, 0, 12);

	Lux_SDL2_SetWindowIcon( native_window );
	SDL_SetWindowTitle( native_window, native_window_title.c_str() );

	SDL_DisableScreenSaver();

	/* Create FBO */

	if ( SDL_GL_ExtensionSupported("GL_EXT_framebuffer_object") )
	{
		glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) SDL_GL_GetProcAddress("glGenFramebuffersEXT");
		glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
		glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
		glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) SDL_GL_GetProcAddress("glBindFramebufferEXT");
		glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");

		glGenFramebuffersEXT(1, &fbo_frame);
		glBindFramebufferEXT(GL_FRAMEBUFFER, fbo_frame);

		for ( uint8_t c = 0; c < 8; c++ )
		{
			opengl_graphic_create_fbotexture( fbo_textures[c] );
		}
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_textures[0].pointer, 0);

		GLenum fbo_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if ( fbo_status != GL_FRAMEBUFFER_COMPLETE_EXT )
		{
			lux::core->SystemMessage( SYSTEM_MESSAGE_LOG ) << "Framebuffer error " << fbo_status << std::endl;
		}
		else
		{
			fbo_supported = true;
		}
		glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

	}
	else
	{
		lux::core->SystemMessage( SYSTEM_MESSAGE_LOG ) << "No Framebuffer Support" << std::endl;
	}

	SDL_CreateWindowAndRenderer(400, 300, SDL_WINDOW_SHOWN, &debug_window, &debug_renderer );

	return true;
}



/* Lux_OGL_Destory
 * Closes down the video mode.
 */
LUX_DISPLAY_FUNCTION void Lux_OGL_Destory()
{
	Lux_GLES_UnloadFont();
	SDL_GL_DeleteContext(native_context);
	SDL_EnableScreenSaver();

	if ( SDL_GL_ExtensionSupported("GL_EXT_framebuffer_object") )
	{
		glDeleteTextures(1, &fbo_textures[0].pointer);
		glDeleteFramebuffersEXT(1, &fbo_frame);
	}
}

/* Lux_OGL_Update
 * Adds an area of the screen that needs to be updated.
 @ rect: area to updates
 */
LUX_DISPLAY_FUNCTION void Lux_OGL_Update(LuxRect rect)
{

}

/* Lux_OGL_Show
 * Refreshs the display
 */
LUX_DISPLAY_FUNCTION void Lux_OGL_Show()
{
	glClearColor((float)gles_graphics_colour.r / 255.0f, (float)gles_graphics_colour.g / 255.0f, (float)gles_graphics_colour.b / 255.0f, 1.0f);

/*
	if ( fbo_supported )
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER, fbo_frame);
		glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
	}
*/
	SDL_RenderPresent(debug_renderer);
	SDL_RenderClear(debug_renderer);


	SDL_GL_SwapWindow(native_window);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

/*
	if ( lux::core )
	{
		if ( lux::core->GetTime() > (sdlgraphics_fpstime + 1000) )
		{
			lux::core->SystemMessage(SYSTEM_MESSAGE_INFO) << "FPS" << sdlgraphics_fps << std::endl;
			sdlgraphics_fpstime = lux::core->GetTime();
			sdlgraphics_fps = 0;
		}
	}
	*/
}

/* Lux_OGL_Display2Screen
 * Converts display location to screen location
 @ x:
 @ y:
 */
LUX_DISPLAY_FUNCTION void Lux_OGL_Display2Screen( int32_t * x, int32_t * y)
{
	*x = (int32_t)((float)*x * opengl_graphic_ratio_width);
	*y = (int32_t)((float)*y * opengl_graphic_ratio_height);
}

/* Lux_OGL_Resize
 *
 @ width:
 @ height:
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_Resize( uint16_t window_width, uint16_t window_height)
{
	native_screen_position.w = window_width;
	native_screen_position.h = window_height;

	SDL_RenderSetLogicalSize(native_renderer, window_width, window_height);

	if ( native_screen_match )
	{
		glViewport(0, 0, window_width, window_height);
		SDL_RenderSetViewport(native_renderer, NULL);

		//gles::setOrtho( 0.0f, (float)window_width, (float)window_height, 0.0f, -10.0f, 10.0f );
	}
	else if ( native_screen_stretching )
	{
		float w_scale = (float)window_width/(float)native_graphics_dimension.w;
		float h_scale = (float)window_height/(float)native_graphics_dimension.h;

		if ( w_scale > h_scale)
		{
			native_screen_position.w = h_scale * native_graphics_dimension.w;
			native_screen_position.h = h_scale * native_graphics_dimension.h;
		}
		else
		{
			native_screen_position.w = w_scale * native_graphics_dimension.w;
			native_screen_position.h = w_scale * native_graphics_dimension.h;
		}
		opengl_graphic_ratio_width = opengl_graphic_ratio_height = 1.0;

		glViewport(0, 0, native_screen_position.w, native_screen_position.h);
		SDL_RenderSetViewport(native_renderer, &native_graphics_dimension);


	}
	else
	{
		opengl_graphic_ratio_width = (float)native_graphics_dimension.w/(float)window_width;
		opengl_graphic_ratio_height = (float)native_graphics_dimension.h/(float)window_height;

		glViewport(0, 0, native_screen_position.w, native_screen_position.h);
		SDL_RenderSetViewport(native_renderer, &native_screen_position);

		//gles::setOrtho( 0.0f, (float)native_graphics_dimension.w, (float)native_graphics_dimension.h, 0.0f, -10.0f, 10.0f );
	}


	if ( window_width > window_height )
	{
		SDL_SetHintWithPriority( SDL_HINT_ORIENTATIONS, "LandscapeLeft", SDL_HINT_OVERRIDE );
	}
	else
	{
		SDL_SetHintWithPriority( SDL_HINT_ORIENTATIONS, "Portrait", SDL_HINT_OVERRIDE );
	}

	return false;
}

/* Lux_OGL_Display2Screen
 * Converts display location to screen location
 @ width:
 @ height:
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_SetFullscreen( bool able )
{

	SDL_SetWindowFullscreen( native_window, ( able ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 ) );

	return true;
}


/* Lux_OGL_BackgroundObject
 * Set the background objects
 @ background:
 */
LUX_DISPLAY_FUNCTION void Lux_OGL_BackgroundObject( MapObject background  )
{
	gles_graphics_colour = background.effects.primary_colour;
	glClearColor((float)gles_graphics_colour.r / 255.0f, (float)gles_graphics_colour.g / 255.0f, (float)gles_graphics_colour.b / 255.0f, 1.0f);
}


/* Lux_OGL_DisplayPointer
 *
 */
LUX_DISPLAY_FUNCTION void Lux_OGL_DisplayPointer( uint8_t player, int16_t x, int16_t y, ObjectEffect effect )
{
	LuxRect position;

	position.x = x;
	position.y = y;
	position.z = 7000;

	Lux_GLES_DrawPolygon(opengl_graphic_cursor->x, opengl_graphic_cursor->y, opengl_graphic_cursor->count, position, effect, NULL);
}

/* Resource Functions */

/* Lux_OGL_FreeSprite
 *
 @ sprite:
 -
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_FreeSprite( LuxSprite * sprite )
{
	if ( sprite == NULL )
		return false;
	if ( sprite->data )
	{
		Texture * texture = (Texture*)sprite->data;
		lux::OpenGLTexture::destroy(texture);
		delete texture;
	}
	sprite->data = NULL;
	return true;
}

/* Lux_OGL_CreateSprite
 *
 @ sprite:
 @ rect:
 @ parent:
 -
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_CreateSprite( LuxSprite * sprite, LuxRect rect, elix::Image * png )
{
	if ( !png->HasContent() )
	{
		lux::core->SystemMessage(SYSTEM_MESSAGE_ERROR) << "PNG is empty" << std::endl;
		return false;
	}

	Texture * texture = new Texture;
	texture->w = sprite->sheet_area.w;
	texture->h = sprite->sheet_area.h;
	if ( Lux_OGL_QueryExtension((char *)"GL_ARB_texture_non_power_of_two") )
	{
		texture->tw = texture->w;
		texture->th = texture->h;
	}
	else
	{
		texture->tw = lux::OpenGLTexture::power(texture->w);
		texture->th = lux::OpenGLTexture::power(texture->h);
	}
	texture->pot = ( texture->w == texture->tw && texture->h == texture->th );

	sprite->data = texture;

	/* PNG image */
	uint32_t * pixels = new uint32_t[texture->tw*texture->th];
	if ( pixels )
	{
		memset(pixels, 0x00, texture->tw * texture->th * 4 );
		for( uint16_t y = 0; y < texture->h; y++ )
		{
			for( uint16_t x = 0; x < texture->w; x++ )
			{
				uint32_t q = (texture->tw * y) + x;
				pixels[q] = png->GetPixel(rect.x + x, rect.y + y);
			}
		}
		lux::OpenGLTexture::create(texture, pixels, texture->tw, texture->th, GLES_TEXTURE_32);
	}
	else
	{
		lux::core->SystemMessage(SYSTEM_MESSAGE_ERROR) << "PNG is empty" << std::endl;
	}
	delete pixels;
	/*  ^ PNG Image */


	return true;
}

/* Lux_OGL_LoadSpriteSheet
 *
 @ name:
 @ children:
 -
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_LoadSpriteSheet( std::string name, std::map<uint32_t, LuxSprite *> * children )
{
	/* PNG Image */
	uint8_t * data = NULL;
	uint32_t size;
	elix::Image * png = new elix::Image;
	if ( lux::game_data )
	{
		size = lux::game_data->GetFile("./sprites/" + name, &data, false);
		if ( size )
		{
			png->LoadFile(data, size);
		}
	}
	delete data;
	/*  ^ PNG Image */

	if ( png->HasContent() )
	{
		std::map<uint32_t, LuxSprite *>::iterator p = children->begin();
		for( ; p != children->end(); p++ )
		{
			if ( !p->second->animated )
			{
				Lux_OGL_CreateSprite( p->second, p->second->sheet_area, png );
			}
		}
		delete png;
		return true;
	}
	else
	{
		return false;
	}
}

/* Lux_OGL_LoadSpriteSheet
 *
 @ name:
 @ children:
 -
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_LoadSpriteSheetImage( elix::Image * image, std::map<uint32_t, LuxSprite *> * children)
{
	if ( !image  )
	{
		return false;
	}

	if (  !children )
	{
		return false;
	}


	if ( image->HasContent() )
	{
		std::map<uint32_t, LuxSprite *>::iterator p = children->begin();
		for( ; p != children->end(); p++ )
		{
			if ( !p->second->animated )
			{
				Lux_OGL_CreateSprite( p->second, p->second->sheet_area, image );
			}
		}
		return true;
	}
	else
	{
		return false;
	}

}

/* Lux_OGL_FreeSpriteSheet
 *
 @ children:
 -
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_FreeSpriteSheet( std::map<uint32_t, LuxSprite *> * children )
{
	std::map<uint32_t, LuxSprite *>::iterator p;
	for( p = children->begin(); p != children->end(); p++ )
	{
		if ( !p->second->animated )
		{
			Lux_OGL_FreeSprite( p->second );
		}
	}
	return true;
}

/* Lux_OGL_RefreshSpriteSheet
 *
 @ name:
 @ children:
 -
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_RefreshSpriteSheet( std::string name, std::map<uint32_t, LuxSprite *> * children )
{
	if ( Lux_OGL_FreeSpriteSheet( children ) )
		Lux_OGL_LoadSpriteSheet( name, children );
	return true;
}

/* Lux_OGL_PNGtoSprite
 *
 @ data:
 @ size:
 -
 */
LUX_DISPLAY_FUNCTION LuxSprite * Lux_OGL_PNGtoSprite( uint8_t * data, uint32_t size )
{
	LuxSprite * sprite = NULL;
	elix::Image * png = new elix::Image(data, size);

	if ( png->HasContent() )
	{
		sprite = new LuxSprite( GraphicsOpenGL );
		sprite->sheet_area.x = sprite->sheet_area.y = 0;
		sprite->sheet_area.w = png->Width();
		sprite->sheet_area.h = png->Height();

		Lux_OGL_CreateSprite( sprite, sprite->sheet_area, png );
	}
	delete png;
	return sprite;

}


/* Lux_OGL_SetRotation
 *
 @ roll:
 @ pitch:
 @ yaw:
 -
 */
LUX_DISPLAY_FUNCTION void Lux_OGL_SetRotation( int16_t roll, int16_t pitch, int16_t yaw )
{

}

/* Lux_OGL_CacheDisplay
 *
 @ layer:
 -
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_CacheDisplay( uint8_t layer )
{
	if ( fbo_supported )
	{
		if ( layer < 8 )
		{


			glBindFramebufferEXT(GL_FRAMEBUFFER, fbo_frame);

			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_textures[layer].pointer, 0);

			GLenum fbo_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
			if ( fbo_status != GL_FRAMEBUFFER_COMPLETE_EXT )
			{
				lux::core->SystemMessage( SYSTEM_MESSAGE_LOG ) << "Framebuffer error " << fbo_status << std::endl;
			}
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glClear( GL_COLOR_BUFFER_BIT );

			return true;
		}
	}

	return false;
}
/*
void Lux_OGL_ScreenSuface( Texture * texture, const char * filename )
{

	int32_t size = texture->tw* texture->th*4;
	uint8_t * data = new uint8_t[size];
	elix::Image * png = new elix::Image();



	glReadPixels(0, 0, texture->tw, texture->th, GL_RGBA, GL_UNSIGNED_BYTE, data );


	png->SetPixels( texture->tw, texture->th, data );

	png->SaveFile( filename );

	delete png;

//	delete [] data;


}
*/

#include <sstream>
/* Lux_OGL_DrawCacheDisplay
 *
 @ layer:
 @ shader:
 -
 */
LUX_DISPLAY_FUNCTION bool Lux_OGL_DrawCacheDisplay( uint8_t layer, uint8_t shader )
{
	if ( fbo_supported )
	{
		LuxVertex scale = { 1, 1, 1 };
		LuxVertex rotation = { 0,0, 0 };
		LuxVertex dest;
		LuxVertex coords[4];
		LuxColour colors[4];


		if ( layer < 8 )
		{
			dest.set( 0, 0, layer );

			OpenGLShader::SetPrimaryColor( 1.0, 1.0, 1.0, 1.00 );
			OpenGLShader::SetSecondaryColor( 0.0, 1.0, 0.0, 1.00 );

			colors[0] = colors[1] = colors[2] = colors[3] = colour::white;

			coords[0].set2( 0, 0, 0, 0.0, 1.0);
			coords[1].set2( fbo_textures[layer].tw, 0, 0.0, 1.0, 1.0 );
			coords[2].set2( 0, fbo_textures[layer].th, 0.0, 0.0, 0.0 );
			coords[3].set2( fbo_textures[layer].tw, fbo_textures[layer].th, 0.0, 1.0, 0.0 );

			glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

			gles::render( GLES_DEFAULT_PRIMITIVE, coords, 4, &fbo_textures[layer], dest, colors, 0, scale, rotation, shader );

		}
	}


	return false;
}




#include "bitfont.h"
static SDL_Texture *gfxPrimitivesFont[128];

SDL_Texture * Lux_OGL_GetCharTexture( uint8_t c )
{
	SDL_Texture * texture;
	uint8_t i = 0, q;
	uint8_t * font_point = &gfxPrimitivesFontdata[0];

	if ( c > 32 && c < 128)
	{
		if ( gfxPrimitivesFont[c] == NULL )
		{
			font_point += (c*8);

			uint16_t * charflip = new uint16_t[64];
			for (i = 0; i < 8; i++)
			{
				for (q = 0; q < 8; q++)
				{
					charflip[(i*8) + q] =  (!!(font_point[i] & (1 << (8-q))) ? 0xFFFF : 0x000F) ;
				}
			}
			texture = SDL_CreateTexture(debug_renderer, SDL_PIXELFORMAT_ARGB1555, SDL_TEXTUREACCESS_STATIC, 8, 8);

			SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
			SDL_UpdateTexture(texture, NULL, charflip, 16);

			delete [] charflip;

			gfxPrimitivesFont[c] = texture;
		}
		else
		{
			texture =  gfxPrimitivesFont[c];
		}
	}
	return texture;
}





void Lux_OGL_SetRectFromText( SDL_Rect & area, std::string text, uint8_t text_width, uint8_t text_height )
{
	uint16_t max_length = 1;
	uint16_t length_count = 0;
	uint16_t lines = 1;
	std::string::iterator object;

	for ( object = text.begin(); object != text.end(); object++ )
	{
		uint8_t utfchar = *object;
		uint32_t cchar = utfchar;
		if ( cchar == '\n' || cchar == '\r' )
		{
			lines++;
			max_length = std::max(length_count, max_length);
			length_count = 0;

		}
		else if ( cchar <= 128 )
		{
			length_count++;
		}
		else if ( cchar < 224 )
		{
			object++;

			length_count++;
		}
		else if ( cchar < 240 )
		{
			object++;
			object++;

			length_count++;
		}
		else if ( cchar < 245 )
		{
			object++;
			object++;
			object++;

			length_count++;
		}
	}
	max_length = std::max(length_count, max_length);

	area.w = text_width * max_length;
	area.h = text_height * lines;

}

LUX_DISPLAY_FUNCTION void Lux_OGL_DrawMessage( std::string message, uint8_t alignment )
{
	std::string::iterator object;
	SDL_Rect draw;
	SDL_Rect rect;
	SDL_Rect area;
	int w,h;

	SDL_GetWindowSize( debug_window, &w, &h );
	rect.x = rect.y = 0;
	Lux_OGL_SetRectFromText( rect, message, 8, 10 );

	area = rect;

	if ( alignment == 3 )
	{
		area.y = h - area.h;
	}
	else if ( alignment == 2 )
	{
		area.y = h - area.h;
		area.x = w - area.w;
	}
	else if ( alignment == 1 )
	{
		area.x = w - area.w;
	}


	SDL_SetRenderDrawColor( debug_renderer, 0, 0, 0,255 );
	SDL_RenderFillRect( debug_renderer, &area );


	draw = area;
	draw.w = draw.h = 8;

	for ( object = message.begin(); object != message.end(); object++ )
	{
		uint8_t utfchar = *object;
		uint32_t cchar = utfchar;
		if (cchar == '\n' || cchar == '\r')
		{
			draw.y += 10;
			draw.x = area.x;
			cchar = 0;
		}
		else if ( cchar <= 32 )
		{
			draw.x += 7;
			cchar = 0;
		}
		else if ( cchar <= 128 )
		{

		}
		else if ( cchar < 224 )
		{
			object++;
			uint32_t next = *object;

			cchar = ((cchar << 6) & 0x7ff) + (next & 0x3f);
		}
		else if ( cchar < 240 )
		{
			uint32_t next;

			object++;
			next = (*object) & 0xff;
			cchar = ((cchar << 12) & 0xffff) + ((next << 6) & 0xfff);

			object++;
			next = (*object) & 0x3f;
			cchar += next;

		}
		else if ( cchar < 245 )
		{
			uint32_t next;

			object++;
			next = (*object) & 0xff;
			cchar = ((cchar << 18) & 0xffff) + ((next << 12) & 0x3ffff);


			object++;
			next = (*object) & 0xff;
			cchar += (next << 6) & 0xfff;

			object++;
			next = (*object) & 0x3f;
			cchar += next;
		}

		if ( cchar != 0 )
		{

			SDL_Texture * texture = NULL;


			if ( cchar >= 32 && cchar <= 128 )
			{
				texture = Lux_OGL_GetCharTexture(cchar);
			}
			else
			{
				texture = Lux_OGL_GetCharTexture('?');
			}

			if ( texture )
			{
				SDL_SetTextureColorMod(texture, 255, 255, 255);
				SDL_SetTextureAlphaMod(texture, 255 );
				SDL_RenderCopy(debug_renderer, texture, NULL, &draw);
			}
			draw.x += 7;

		}
	}

}
