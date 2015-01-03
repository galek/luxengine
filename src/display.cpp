/****************************
Copyright © 2006-2015 Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
****************************/
#include <cstdlib>

#include "display.h"
#include "core.h"
#include "game_config.h"
#include "engine.h"
#include "game_system.h"
#include "lux_canvas.h"
#include "elix_string.hpp"
#include "mokoi_game.h"
#include "display_types.h"

extern GraphicSystem GraphicsNone;
extern GraphicSystem GraphicsNative;
#ifdef OPENGLENABLED
extern GraphicSystem GraphicsOpenGL;
#endif

std::string static_messages_text;
uint8_t static_messages_count = 0;

void MessagePush( char * message, ... )
{
	char * text = new char[128];
	va_list args;
	va_start( args, message );
	vsnprintf( text, 127, message, args );
	va_end(args);

	static_messages_text.append(text);
	static_messages_text.append("\n");
	static_messages_count++;

	if ( static_messages_count == 13 )
	{
		static_messages_text.erase(0, static_messages_text.find("\n") + 1);
		static_messages_count--;
	}

	delete [] text;
}



void Lux_DisplayMessage( std::string reason )
{
	if ( lux::display )
	{
		lux::display->debug_msg << reason << std::endl;
	}
}

DisplaySystem::DisplaySystem()
{
	bool is_display_setup = false;

	this->InitialSetup();
	this->Init();

	#if DISPLAYMODE_OPENGL
	if ( GraphicsOpenGL.InitGraphics( this->title, this->screen_dimension.w, this->screen_dimension.h, this->bpp, this->fullscreen ) )
	{
		this->graphics = GraphicsOpenGL;
		is_display_setup = true;
	}
	#else
	if ( GraphicsNative.InitGraphics( this->title, this->screen_dimension.w, this->screen_dimension.h, this->bpp, this->fullscreen ) )
	{
		this->graphics = GraphicsNative;
		is_display_setup = true;
	}
	#endif

	if ( !is_display_setup )
	{
		lux::core->SystemMessage(__FILE__, __LINE__, SYSTEM_MESSAGE_INFO) << " Graphic System Failed" << std::endl;
		return;
	}

	this->cache_sprites = (this->graphics.CacheSprite);
	if ( lux::media != NULL )
		lux::media->Init(this->graphics);

}

DisplaySystem::DisplaySystem( std::string title, uint16_t width, uint16_t height, uint8_t bpp, bool fullscreen  )
{
	bool is_display_setup = false;

	this->InitialSetup();

	this->screen_dimension.w = width;
	this->screen_dimension.h = height;
	this->bpp = bpp;
	this->fullscreen = fullscreen;

	#if DISPLAYMODE_OPENGL
	if ( GraphicsOpenGL.InitGraphics( title, width, height, bpp, fullscreen ) )
	{
		this->graphics = GraphicsOpenGL;
		is_display_setup = true;
	}
	#else
	if ( GraphicsNative.InitGraphics( title, width, height, bpp, fullscreen ) )
	{
		this->graphics = GraphicsNative;
		is_display_setup = true;
	}
	#endif

	if ( !is_display_setup )
	{
		lux::core->SystemMessage(__FILE__, __LINE__, SYSTEM_MESSAGE_INFO) << " Graphic System Failed" << std::endl;
		return;
	}

	this->cache_sprites = (this->graphics.CacheSprite);
	if ( lux::media != NULL )
		lux::media->Init( this->graphics );
}

DisplaySystem::~DisplaySystem()
{
	delete this->overlay_layer;

	if ( lux::media != NULL )
		lux::media->Free();
	this->Close();
}

void DisplaySystem::InitialSetup()
{
	if ( lux::core == NULL )
	{
		return;
	}

	this->title = "";
	this->frame_skip = 0;
	this->screen_dimension.x = 0;
	this->screen_dimension.y = 0;
	this->sprite_font = "";
	this->show_cursor = true;
	this->cache_sprites = false;

	this->graphics = GraphicsNone;
	this->overlay_layer = new Layer(this, 7, true);

}

/**/
bool DisplaySystem::Init()
{
	this->graphics.TextSprites( false );

	if ( lux::config )
	{
		this->title = lux::config->GetString("project.title");
		this->screen_dimension.w = lux::config->GetNumber("screen.width");
		this->screen_dimension.h = lux::config->GetNumber("screen.height");
		this->bpp = (uint8_t)lux::config->GetNumber("display.bpp") * 8;
		this->fullscreen = lux::config->GetBoolean("display.fullscreen");
		this->sprite_quick_access = lux::config->GetBoolean("sprite.shortname");
		this->show_3d = lux::config->GetBoolean("fun.able");
		this->show_debug = lux::config->GetBoolean("debug.able");
		this->show_mask = lux::config->GetBoolean("debug.masks");
		this->show_spriteinfo = lux::config->GetBoolean("debug.sprite");
		this->show_collisions = lux::config->GetBoolean("debug.collision");
	}

	/* Create Layers */
	for (uint8_t i = 0; i < 7; i++)
	{
		std::string layername = "layer.static";
		layername.append( 1, i + 48 );

		this->show_layers[i] = true;

		this->_layers.push_back( new Layer( this, i, lux::config->GetBoolean(layername) ) );
	}

	/* Preload Spritesheets */
	std::stringstream sprite_file;
	std::string buffer;
	if ( lux::game_data->GetStream("./preload/spritesheets.txt", &sprite_file) )
	{
		do
		{
			std::getline (sprite_file, buffer);
			elix::string::Trim(&buffer);
			if ( buffer.length() )
			{
				lux::screen::display("Loading Spritesheet: " + buffer);
				this->RefSheet(buffer);
			}
		} while ( !sprite_file.eof() );
		sprite_file.clear();
	}
	return true;
}

void DisplaySystem::Loop(LuxState engine_state)
{
	int i = 0;
	std::vector<Layer *>::iterator p;
	for ( p = this->_layers.begin(); p != this->_layers.end(); p++ )
	{

		Layer * current_layer = (*p);
		if ( this->show_layers[i] )
		{
			if ( current_layer->shader )
			{
				this->graphics.CacheDisplay( current_layer->display_layer );
				current_layer->Display();

				this->debug_msg << "Layer: " << (int)current_layer->display_layer << " using FBO" << std::endl;

				this->graphics.DrawCacheDisplay( current_layer->display_layer, current_layer->shader );
			}
			else
			{
				if ( this->graphics.SetRotation != NULL &&  this->show_3d  )
				{
					this->graphics.SetRotation( current_layer->_roll, current_layer->_pitch, current_layer->_yaw );
					current_layer->Display();
					this->graphics.SetRotation(0,0,0);
				}
				else
				{
					current_layer->Display();
				}
			}
		}
		i++;
	}

	this->DisplayOverlay();

	this->DrawCursor();
	this->ShowMessages();
	this->graphics.Show();
}

void DisplaySystem::DrawGameStatic()
{
	int i = 0;
	std::vector<Layer *>::iterator p;
	for ( p = this->_layers.begin(); p != this->_layers.end(); p++ )
	{
		Layer * current_layer = (*p);
		if ( this->show_layers[i] )
		{
			if ( this->graphics.SetRotation != NULL &&  this->show_3d  )
			{
				this->graphics.SetRotation( current_layer->_roll, current_layer->_pitch, current_layer->_yaw );
				current_layer->Display();
				this->graphics.SetRotation(0,0,0);
			}
			else
			{
				current_layer->Display();
			}
		}
		i++;
	}
	DisplayOverlay();
}

void DisplaySystem::DisplayOverlay()
{
	if ( this->overlay_layer )
	{
		this->overlay_layer->Display();
	}

}

bool DisplaySystem::Close()
{
	this->ClearLayers(false);

	while ( this->_layers.begin() != this->_layers.end())
	{
		delete (*this->_layers.begin());
		this->_layers.erase( this->_layers.begin() );
	}
	this->_layers.clear();

	if ( this->_sheets.size() )
	{
		for ( LuxSheetIter iter_sheet = this->_sheets.begin(); iter_sheet != this->_sheets.end(); iter_sheet++ )
		{
			iter_sheet->second->Unload();
		}
	}
	this->_sheets.clear();


	if ( this->graphics.DestoryGraphics )
	{
		this->graphics.DestoryGraphics();
	}


	return true;
}

void DisplaySystem::DrawCursor()
{
	if ( this->show_cursor && lux::engine )
	{
		std::vector<Player *>::iterator iter = lux::engine->_players.begin();
		while( iter != lux::engine->_players.end() )
		{
			this->graphics.DisplayPointer( (*iter)->_id, (*iter)->GetPointer(0), (*iter)->GetPointer(1), (*iter)->PlayerColour );
			iter++;
		}
	}

}

void DisplaySystem::ShowMessages()
{
	ObjectEffect text_effects;
	ObjectEffect rect_effects;

	text_effects.SetColour(colour::yellow);
	rect_effects.SetColour(colour::black);
	rect_effects.primary_colour.a = 128;

	if ( this->show_collisions )
	{
		lux::gamesystem->GetObjects()->DrawCollisions();
	}


	if ( this->show_mask )
	{
		lux::gamesystem->active_map->DrawMask();
	}


	if ( this->show_debug )
	{
		this->graphics.DrawMessage( this->debug_msg.str(), 0 );
		this->graphics.DrawMessage( static_messages_text, 3 );
	}
	this->debug_msg.str("");

}






void DisplaySystem::SetTextFont(bool enable, std::string font_sheet )
{
	if ( this->sprite_font.length() )
	{
		this->UnrefSheet(lux::display->sprite_font);
	}
	this->RefSheet( font_sheet );

	this->sprite_font = font_sheet;

	this->graphics.TextSprites( enable );
}


/* Spritesheet Management */

void DisplaySystem::ReloadSheets()
{
	LuxSheetIter iter_sheet;
	for ( iter_sheet = this->_sheets.begin(); iter_sheet != this->_sheets.end(); iter_sheet++ )
	{
		iter_sheet->second->Refresh( this->graphics );
	}
}

void DisplaySystem::LoadSheet( LuxSheet * sheet )
{
	if ( sheet != NULL )
	{
		sheet->Load();
	}
}

LuxSheet * DisplaySystem::GetSheet( std::string name )
{
	LuxSheet * sheet = NULL;
	LuxSheetIter iter_sheet = this->_sheets.find(name);

	if ( iter_sheet == this->_sheets.end() )
	{
		sheet = new LuxSheet( name, this->graphics );
		this->_sheets.insert( std::pair<std::string, LuxSheet*>(name, sheet) );
	}
	else
	{
		sheet = iter_sheet->second;
	}

	return sheet;
}


void DisplaySystem::LoadSheet( std::string name, bool preload )
{
	LuxSheet * sheet = this->GetSheet( name );
	if ( sheet )
	{
		sheet->Load();
		if ( preload )
		{
			sheet->Ref();
		}
	}
}

void DisplaySystem::RefSheet(std::string name)
{
	LuxSheetIter iter_sheet = this->_sheets.find(name);

	if ( iter_sheet == this->_sheets.end() )
	{
		this->LoadSheet( name, true );
		return;
	}

	if ( iter_sheet->second != NULL )
	{
		iter_sheet->second->Ref();
	}

}

void DisplaySystem::UnrefSheet(std::string name)
{
	LuxSheetIter iter_sheet = this->_sheets.find(name);

	if ( iter_sheet == this->_sheets.end() )
	{
		lux::core->SystemMessage(SYSTEM_MESSAGE_INFO) << "RefSheet Failed: " << name << std::endl;
		return;
	}

	if ( iter_sheet->second != NULL )
	{
		iter_sheet->second->Unref();
	}
}

void DisplaySystem::UnloadSheet(std::string name)
{
	LuxSheetIter iter_sheet = this->_sheets.find(name);

	if ( iter_sheet == this->_sheets.end() )
	{
		return;
	}

	if ( iter_sheet->second != NULL )
	{
		if ( iter_sheet->second->Unload() )
		{
			this->_sheets.erase(iter_sheet);
		}
	}
}

/* Spritesheet Management */

LuxSprite * DisplaySystem::GetSprite( std::string parent, std::string name )
{
	LuxSprite * sprite = NULL;
	LuxSheet * sheet = this->GetSheet( parent );

	if ( sheet )
	{
		sprite = sheet->GetSprite( name );
	}
	return sprite;
}

LuxSprite * DisplaySystem::GetSprite( std::string parent, uint32_t name_hash )
{
	LuxSprite * sprite = NULL;
	LuxSheet * sheet = this->GetSheet( parent );

	if ( sheet )
	{
		sprite = sheet->GetSprite( name_hash );
	}
	return sprite;
}

LuxSprite * DisplaySystem::GetSprite( std::string sprite_name )
{
	LuxSprite * sprite = NULL;
	if ( this->sprite_quick_access )
	{
		std::map<uint32_t, LuxSprite *>::iterator iter = this->_sprites.find( elix::string::Hash(sprite_name) );
		if( iter != this->_sprites.end() )
		{
			return (LuxSprite *) iter->second;
		}
	}
	else
	{
		std::vector<std::string> name_split;
		elix::string::Split(sprite_name, ":", &name_split);
		if ( name_split.size() == 2 )
		{
			sprite = this->GetSprite(name_split[0], name_split[1]);
		}
		name_split.clear();
	}
	return sprite;
}

uint32_t DisplaySystem::isAnimation(std::string parent, std::string name)
{
	LuxSprite * sprite = this->GetSprite(parent, name);
	if ( sprite )
	{
		if ( sprite->animated )
		{
			return sprite->animation_length;
		}
	}
	return 0;
}


void DisplaySystem::DrawMapObjectBorder( uint8_t position, SpriteBorder sprite_border, LuxRect object, ObjectEffect new_effects )
{
	LuxRect rect = object;

	rect.w = sprite_border.sprite->sheet_area.w;
	rect.h = sprite_border.sprite->sheet_area.h;
	switch ( position )
	{
		case 0:// "top-left"
		{
			rect.x -= sprite_border.sprite->sheet_area.w;
			rect.y -= sprite_border.sprite->sheet_area.h;

			break;
		}
		case 1:// "top"
		{
			rect.y -= sprite_border.sprite->sheet_area.h;
			if ( sprite_border.repeat ) // Repeat
			{
				new_effects.tile_object = true;
				rect.w = object.w;
			}
			break;
		}
		case 2:// top-right"
		{
			rect.x += object.w;
			rect.y -= sprite_border.sprite->sheet_area.h;
			break;
		}
		case 3:// "right"
		{
			rect.x += object.w;
			if ( sprite_border.repeat ) // Repeat
			{
				new_effects.tile_object = true;
				rect.h = object.h;
			}
			break;
		}
		case 4:// "bottom-right"
		{
			rect.x += object.w;
			rect.y += object.h;
			break;
		}
		case 5: //"bottom"
		{
			rect.y += object.h;
			if ( sprite_border.repeat ) // Repeat
			{
				new_effects.tile_object = true;
				rect.w = object.w;
			}
			break;
		}
		case 6: //"bottom-left"
		{
			rect.x -= sprite_border.sprite->sheet_area.w;
			rect.y += object.h;
			break;
		}
		case 7: //"left"
		{
			rect.x -= sprite_border.sprite->sheet_area.w;
			if ( sprite_border.repeat ) // Repeat
			{
				new_effects.tile_object = true;
				rect.h = object.h;
			}
			break;
		}
		default:
		break;
	}
	this->graphics.DrawSprite( sprite_border.sprite, rect, new_effects );
}


void DisplaySystem::DrawMapObject( MapObject * object, LuxRect new_position, ObjectEffect new_effects )
{
	if ( !object )
	{
		return;
	}

	std::stringstream msg;
	switch ( object->type )
	{
		case OBJECT_SPRITE:
		{
			LuxSprite * sprite = object->GetCurrentSprite();
			if ( sprite )
			{
				new_position.x += object->offset_x;
				new_position.y += object->offset_y;
				this->graphics.DrawSprite( sprite, new_position, new_effects );

				if ( sprite->has_sprite_border )
				{
					for ( uint8_t p = 0; p < 8; p++ )
					{
						if ( sprite->border[p].sprite )
						{
							this->DrawMapObjectBorder( p, sprite->border[p], new_position, new_effects );
						}
					}
				}

				if ( this->show_spriteinfo )
				{
					msg << object->GetInfo() << std::endl;
					this->graphics.DrawText( msg.str(), new_position, default_fx, false );
				}
				if ( this->show_collisions )
				{

					if ( object->_path.size() )
					{
						LuxPath points = { 0, 0, 0 };
						for ( uint16_t p = 0; p < object->_path.size(); p++ )
						{
							points = object->_path.at(p);
							new_position.x = points.x;
							new_position.y = points.y;
							new_position.z = 6999;
							new_position.w = new_position.h = 4;
							this->graphics.DrawRect(new_position, new_effects);
						}
					}
				}
			}

			break;
		}
		case 'r':
			this->graphics.DrawRect(new_position, new_effects);
			break;
		case 'c':
			this->graphics.DrawCircle(new_position, new_effects);
			break;
		case 'M':
		{
			LuxCanvas * canvas = object->GetCanvas();
			if ( canvas )
				canvas->Draw( this, new_position.x, new_position.y, new_position.z );
			break;
		}
		case 'p':
		{
			LuxPolygon * poly = object->GetPolygon();
			if ( poly )
				this->graphics.DrawPolygon(poly->x, poly->y, poly->count, new_position, new_effects, poly->texture);
			break;
		}
		case 'l':
			this->graphics.DrawLine(new_position, new_effects);
			break;
		case 't':
			this->graphics.DrawText(object->sprite, new_position, new_effects, true);
			break;
		default:
			break;
	}
}

LuxSprite * DisplaySystem::GetInputSprite(uint32_t player_id, int8_t axis, int8_t key, int8_t pointer )
{
	if ( !lux::engine )
		return NULL;

	LuxSprite * sprite = NULL;
	Player * player = NULL;
	player = lux::engine->GetPlayer(player_id);

	if ( player )
		sprite = player->GetInputSprite(axis, key, pointer);

	return sprite;
}



/* Layer System */
Layer * DisplaySystem::GetLayer( uint32_t layer )
{
	Layer * requested_layer = NULL;

	if ( layer < this->_layers.size() )
	{
		requested_layer = this->_layers[layer];

	}

	return requested_layer;
}

bool DisplaySystem::AddObjects(std::list<MapObject*> * objects)
{
	std::list<MapObject*>::iterator object;
	for ( object = objects->begin(); object != objects->end(); object++ )
	{
		if ( (*object)->layer < this->_layers.size() )
			this->_layers[(*object)->layer]->AddObject(*object, true);
	}
	return false;
}
Layer * last_layer = NULL;
bool DisplaySystem::AddObjectToLayer(uint32_t layer, MapObject * new_object, bool static_objects)
{
	if ( layer == 0xFFFFFFFF )
	{
		return this->overlay_layer->AddObject(new_object, static_objects);
	}
	else if ( layer < this->_layers.size() )
	{
		Layer * selected_layer = this->_layers.at(layer);
		last_layer = selected_layer;
		return selected_layer->AddObject(new_object, static_objects);
	}
	else
	{
		MessagePush( (char *)"Failed to Add '%s' to layer %d", new_object->TypeName().c_str(), layer );
	}
	return false;
}

bool DisplaySystem::RemoveObject(uint32_t layer, MapObject * new_object)
{
	if ( layer < this->_layers.size() )
	{
		return this->_layers[layer]->RemoveObject(new_object);
	}
	return false;
}

void DisplaySystem::CacheLayers( bool able )
{
	for (uint32_t i = 0; i < this->_layers.size(); i++)
	{
		this->_layers[i]->_cachedynamic = able;
	}
}

bool DisplaySystem::ClearLayer(uint32_t layer, bool static_objects)
{
	if ( layer < this->_layers.size() )
		return this->_layers[layer]->ClearObjects(static_objects);
	return false;
}

bool DisplaySystem::ClearLayers( bool skip_static )
{
	for ( uint32_t i = 0; i < this->_layers.size(); i++ )
	{
		this->_layers[i]->ClearAll();
	}
	return true;
}

void DisplaySystem::ResortLayer( uint32_t layer )
{
	if ( layer < this->_layers.size() )
	{
		this->_layers[layer]->refreshz = true;
	}
}

/* Camera Functions */
void DisplaySystem::SetCameraView( uint32_t layer, fixed x, fixed y )
{
	if ( layer < this->_layers.size() )
	{
		this->_layers[layer]->SetOffset( x, y);
	}
}

void DisplaySystem::SetCameraView( fixed x, fixed y )
{
	for (uint32_t i = 0; i < this->_layers.size(); i++)
	{
		this->_layers[i]->SetOffset( x, y);
	}
}

void DisplaySystem::ChangeLayerRotation( int16_t roll, int16_t pitch, int16_t yaw )
{
	for (uint32_t i = 0; i < this->_layers.size(); i++)
	{
		this->_layers[i]->SetRotation(roll, pitch, yaw);
	}
}

void DisplaySystem::ChangeLayerRotation( uint32_t layer, int16_t roll, int16_t pitch, int16_t yaw )
{
	if ( layer < this->_layers.size() )
	{
		this->_layers[layer]->SetRotation(roll, pitch, yaw);
	}
}

/* */

void DisplaySystem::SetLayerColour( uint32_t layer,  LuxColour colour )
{
	if ( layer < this->_layers.size() )
	{
		this->_layers[layer]->colour = colour;
		if ( layer == 0 && lux::gamesystem->active_map )
			lux::gamesystem->active_map->SetBackgroundModifier( colour );
	}
}


bool DisplaySystem::SetCursor( bool able )
{
	this->show_cursor = able;
	return this->show_cursor;
}


void DisplaySystem::SetBackgroundColour( LuxColour fillcolor )
{
	this->background_object.effects.primary_colour = fillcolor;
	this->graphics.SetBackground( this->background_object );
}

void DisplaySystem::SetBackgroundEffect( ObjectEffect effect )
{
	this->background_object.effects = effect;
	this->graphics.SetBackground( this->background_object );
}

void DisplaySystem::SetBackgroundObject( MapObject background )
{
	this->background_object = background;
	this->graphics.SetBackground( this->background_object );
}

void DisplaySystem::ResetBackgroundObject( )
{
	this->background_object.effects.primary_colour = colour::black;
	this->background_object.effects.SetSecondaryColour( colour::black );
	this->graphics.SetBackground( this->background_object );
}
