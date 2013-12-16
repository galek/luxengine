/****************************
Copyright © 2006-2011 Luke Salisbury
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
****************************/
#include "luxengine.h"
#include "map_object.h"
#include "elix_string.hpp"
#include "lux_canvas.h"
#include "display.h"
#include "core.h"

extern ObjectEffect default_fx;

MapObject::MapObject()
{
	this->effects = default_fx;
	this->effects.tile_object = true;
	this->data = NULL;
	this->data_type = 0;
	this->timer = 0;
	this->speed = 1;
	this->has_data = false;
	this->static_map_id = 0;
	this->hidden = false;
	this->loop = true;
	this->reverse = false;
	this->offset_x = this->offset_y = 0;
	this->sprite_width = 0;
	this->sprite_height = 0;
	this->path_point = 0;
	this->image = "";
	this->layer_ref = 0;
	this->speed = 1;
	this->can_remove = false;
	this->position.x = this->position.y = 0;
	this->position.w = this->position.h = 0;
	this->position.z = 0;
	this->layer = 0;
	this->timer_mode = 0;
	this->auto_delete = 0;
	this->path_current_x = this->path_current_y = 0;
	this->global = false;
}

MapObject::~MapObject()
{
	this->FreeData();
}

void MapObject::Save(elix::File *current_save_file)
{
	current_save_file->WriteWithLabel("Map Object Type", this->type );
	current_save_file->WriteWithLabel("Map Object Timer", this->timer );
	current_save_file->WriteWithLabel("Map Object Timer Mode", this->timer_mode );
	current_save_file->WriteWithLabel("Map Object X", (uint32_t)this->position.x );
	current_save_file->WriteWithLabel("Map Object Y", (uint32_t)this->position.y );
	current_save_file->WriteWithLabel("Map Object W", this->position.w );
	current_save_file->WriteWithLabel("Map Object H", this->position.h );
	current_save_file->WriteWithLabel("Map Object Z", (uint16_t)this->position.z );
	current_save_file->WriteWithLabel("Map Object r1", this->effects.primary_colour.r );
	current_save_file->WriteWithLabel("Map Object b1", this->effects.primary_colour.b );
	current_save_file->WriteWithLabel("Map Object g1", this->effects.primary_colour.g );
	current_save_file->WriteWithLabel("Map Object a1", this->effects.primary_colour.a );
	current_save_file->WriteWithLabel("Map Object r2", this->effects.secondary_colour.r );
	current_save_file->WriteWithLabel("Map Object b2", this->effects.secondary_colour.b );
	current_save_file->WriteWithLabel("Map Object g2", this->effects.secondary_colour.g );
	current_save_file->WriteWithLabel("Map Object a2", this->effects.secondary_colour.a );
	current_save_file->WriteWithLabel("Map Object rotation", this->effects.rotation );
	current_save_file->WriteWithLabel("Map Object scale x", this->effects.scale_xaxis );
	current_save_file->WriteWithLabel("Map Object scale y", this->effects.scale_yaxis );
	current_save_file->WriteWithLabel("Map Object flip mode", this->effects.flip_image );
	current_save_file->WriteWithLabel("Map Object tiling", this->effects.tile_object );
	current_save_file->WriteWithLabel("Map Object style", this->effects.style );
	current_save_file->WriteWithLabel("Map Object current path point", this->path_point );
	current_save_file->WriteWithLabel("Map Object Image", this->image );

}

void MapObject::Restore( elix::File * current_save_file )
{
	this->type = current_save_file->Read_uint8WithLabel("Map Object Type" );
	this->timer = current_save_file->Read_uint32WithLabel("Map Object Timer", true );
	this->timer_mode = current_save_file->Read_uint8WithLabel("Map Object Timer Mode");
	this->position.x = (int32_t)current_save_file->Read_uint32WithLabel("Map Object X", true );
	this->position.y = (int32_t)current_save_file->Read_uint32WithLabel("Map Object Y", true );
	this->position.w = current_save_file->Read_uint16WithLabel("Map Object W", true );
	this->position.h = current_save_file->Read_uint16WithLabel("Map Object H", true );
	this->position.z = (int16_t)current_save_file->Read_uint16WithLabel("Map Object Z", true );
	this->effects.primary_colour.r = current_save_file->Read_uint8WithLabel("Map Object r1" );
	this->effects.primary_colour.b = current_save_file->Read_uint8WithLabel("Map Object g1" );
	this->effects.primary_colour.g = current_save_file->Read_uint8WithLabel("Map Object b1" );
	this->effects.primary_colour.a = current_save_file->Read_uint8WithLabel("Map Object a1" );
	this->effects.secondary_colour.r = current_save_file->Read_uint8WithLabel("Map Object r2" );
	this->effects.secondary_colour.b = current_save_file->Read_uint8WithLabel("Map Object g2" );
	this->effects.secondary_colour.g = current_save_file->Read_uint8WithLabel("Map Object b2" );
	this->effects.secondary_colour.a = current_save_file->Read_uint8WithLabel("Map Object a2" );
	this->effects.rotation = current_save_file->Read_uint16WithLabel("Map Object rotation", true );
	this->effects.scale_xaxis = current_save_file->Read_uint16WithLabel("Map Object scale x", true );
	this->effects.scale_yaxis = current_save_file->Read_uint16WithLabel("Map Object scale y", true );
	this->effects.flip_image = current_save_file->Read_uint8WithLabel("Map Object flip mode" );
	this->effects.tile_object = current_save_file->Read_uint8WithLabel("Map Object tiling" );
	this->effects.style = current_save_file->Read_uint8WithLabel("Map Object style" );
	this->path_point = 0;current_save_file->Read_uint16WithLabel("Map Object current path point", true );
	current_save_file->ReadWithLabel("Map Object Image", &this->image );

	if ( this->type == 's' )
	{
		this->SetData(NULL, this->type);
	}
}

void MapObject::Toggle()
{
	this->hidden = !this->hidden;
}

mem_pointer MapObject::GetData()
{
	return this->data;
}

void MapObject::SetData(void * data, uint8_t type)
{
	if ( !data )
	{
		if ( type == 's' )
		{
			this->has_data = false;
			std::vector<std::string> name_split;
			elix::string::Split(this->image, ":", &name_split);
			if ( name_split.size() == 2 )
			{
				LuxSprite * sdata = lux::display->GetSprite( name_split[0], name_split[1]);
				if ( sdata )
				{
					this->data = (void*)sdata;
					this->type = type;
					this->data_type = 's';
					this->has_data = true;
					this->sprite_width = sdata->sheet_area.w;
					this->sprite_height = sdata->sheet_area.h;
					if ( (this->position.w == sdata->sheet_area.w) && (this->position.h == sdata->sheet_area.h) )
					{
						//this->position.w = this->position.h = 0;
						this->effects.tile_object = 0;
					}
					if ( !this->position.w )
						this->position.w = sdata->sheet_area.w;
					if ( !this->position.h )
						this->position.h = sdata->sheet_area.h;

					if ( sdata->has_sprite_border )
					{
						for ( uint8_t p = 0; p < 8; p++ )
						{
							if ( sdata->border[p].name )
								sdata->border[p].sprite = lux::display->GetSprite( name_split[0], sdata->border[p].name );

						}
					}

				}
				else
				{
					MessagePush( (char*)"Sprite missing: %s", this->image.c_str() );
				}
			}
			else
			{
				this->data_type = 0;
			}
			name_split.clear();
		}
		else if ( type == 'M' )
		{
			this->SetCanvas( new LuxCanvas(this->image) );
		}
		else if ( type == 'p' )
		{
			//this->SetPolygon( new LuxPolygon(this->image) );
		}
		else if ( type == 't' )
		{
			this->has_data = true;
			this->data = data;
			this->data_type = 't';
		}
		else
		{
			this->data_type = type;
			this->has_data = true;
		}
	}
	else
	{
		this->data = data;
		this->data_type = type;
		this->has_data = true;
	}
}

void MapObject::FreeData()
{
	this->layer_ref = 0;
	if ( !this->has_data )
		return;
	if ( this->type == 's' )
	{
		lux::display->UnrefSheet( this->image.substr(0, this->image.find_first_of(':')) );
	}
	else if ( this->data_type == 't' )
	{

	}
	else if ( this->data_type == 'p' )
	{
		if ( this->data )
		{
			delete (LuxPolygon*)this->data;
		}
	}
	else if ( this->data_type == 'M' )
	{
		if ( this->data )
		{
			delete (LuxCanvas*)this->data;
		}
	}
	this->data = NULL;
	this->data_type = 0;
	this->has_data = false;
}


/* Sprites */
void MapObject::IncrementAnimation( LuxSprite * sprite )
{
	if ( !sprite->animated || !sprite->animation_size )
	{
		return;
	}
	this->timer += lux::core->GetAnimationDelta() * this->speed;

	if ( this->timer >= sprite->animation_length )
	{
		if ( this->loop )
			this->timer -= sprite->animation_length;
		else
			this->timer = sprite->animation_length-1;
	}

}

SpriteFrame MapObject::RetieveAnimationFrame( LuxSprite * sprite, uint32_t timer )
{
	std::vector<SpriteFrame>::iterator iter_forward;
	std::vector<SpriteFrame>::reverse_iterator iter_rev;
	SpriteFrame current_frame = { NULL, 0, 0, 0 };
	uint32_t count = 0, next_count = 0;

	if ( this->reverse )
	{
		for ( iter_rev = sprite->frames.rbegin(); iter_rev != sprite->frames.rend(); ++iter_rev )
		{
			next_count += (*iter_rev).ms;
			if ( timer <= next_count && timer >= count )
			{
				current_frame = (*iter_rev);
				break;
			}
			count = next_count;
		}
	}
	else
	{
		for ( iter_forward = sprite->frames.begin(); iter_forward != sprite->frames.end(); ++iter_forward )
		{
			next_count += (*iter_forward).ms;
			if ( timer <= next_count && timer >= count )
			{
				current_frame = (*iter_forward);
				break;
			}
			count = next_count;
		}
	}
	return current_frame;
}

LuxSprite * MapObject::PeekAnimationFrame( LuxSprite * sprite )
{
	if ( !sprite->animated || !sprite->animation_length )
	{
		return sprite;
	}

	SpriteFrame current_frame;
	uint32_t timer = this->timer;

	timer += lux::core->GetAnimationDelta() * this->speed;

	if ( timer >= sprite->animation_length )
	{
		if ( this->loop )
			timer -= sprite->animation_length;
		else
			timer = sprite->animation_length-1;
	}

	current_frame = this->RetieveAnimationFrame( sprite, timer );

	if ( current_frame.sprite )
	{
		if ( this->effects.flip_image&16 )
			this->offset_x = -current_frame.x;
		else
			this->offset_x = current_frame.x;
		this->offset_y = current_frame.y;
		return current_frame.sprite;
	}
	return sprite;
}

LuxSprite * MapObject::GetAnimationFrame( LuxSprite * sprite, bool no_increment )
{
	if ( !sprite->animated || !sprite->animation_length )
	{
		return sprite;
	}

	SpriteFrame current_frame;

	if ( !no_increment )
	{
		this->IncrementAnimation( sprite );
	}

	current_frame = this->RetieveAnimationFrame( sprite, this->timer );


	if ( current_frame.sprite )
	{
		if ( this->effects.flip_image&16 )
			this->offset_x = -current_frame.x;
		else
			this->offset_x = current_frame.x;
		this->offset_y = current_frame.y;
		return current_frame.sprite;
	}
	return sprite;
}

LuxSprite * MapObject::PeekSprite(  )
{
	if ( this->data )
	{
		if ( this->data_type == 's' )
		{
			LuxSprite * orig = (LuxSprite*)this->data;

			if ( !orig )
				return NULL;

			if ( !orig->animated )
				return orig;
			else
				return this->PeekAnimationFrame( orig );
		}
	}
	return NULL;
}


LuxSprite * MapObject::GetSprite( bool no_increment )
{
	this->offset_x = 0;
	this->offset_y = 0;
	if ( this->data )
	{
		if ( this->data_type == 's' )
		{
			LuxSprite * orig = (LuxSprite*)this->data;

			if ( !orig )
				return NULL;

			if ( !orig->animated )
				return orig;
			else
				return this->GetAnimationFrame( orig, no_increment );
		}
		else
		{
			lux::core->SystemMessage(SYSTEM_MESSAGE_INFO) << "data type error" << std::endl;
		}
	}
	return NULL;
}

LuxSprite * MapObject::GetCurrentSprite( )
{
	this->offset_x = 0;
	this->offset_y = 0;
	if ( !this->data )
	{
		this->SetData(NULL, this->type);
	}

	if ( this->data )
	{
		if ( this->data_type == 's' )
		{
			LuxSprite * orig = (LuxSprite*)this->data;
			if ( !orig )
				return NULL;

			if ( orig->animated )
			{
				LuxSprite * sprite = this->GetAnimationFrame( orig, false );
				if ( sprite )
				{
					if ( !this->position.w )
						this->position.w = sprite->sheet_area.w;
					if ( !this->position.h )
						this->position.h = sprite->sheet_area.h;
					return sprite;
				}
			}

			return orig;
		}
		else
		{
			lux::core->SystemMessage(SYSTEM_MESSAGE_INFO) << "data type error" << std::endl;
		}
	}
	return NULL;
}

mem_pointer MapObject::GetImage( ObjectEffect fx )
{
	this->offset_x = 0;
	this->offset_y = 0;
	if ( !this->data )
	{
		this->SetData(NULL, this->type);
	}

	if ( this->data )
	{
		if ( this->data_type == 's' )
		{
			LuxSprite * orig = (LuxSprite*)this->data;
			if ( !orig )
				return NULL;

			if ( orig->animated )
			{
				LuxSprite * sprite = this->GetAnimationFrame( orig, false );
				if ( sprite )
				{
					if ( !this->position.w )
						this->position.w = sprite->sheet_area.w;
					if ( !this->position.h )
						this->position.h = sprite->sheet_area.h;
					return sprite->GetData(fx);
				}
			}
			else
				return orig->GetData(fx);

		}
		else
			return this->data;
	}
	return NULL;
}



/* Polygon */

LuxPolygon * MapObject::GetPolygon()
{
	if (this->data_type == 'p' && this->has_data)
	{
		return (LuxPolygon*)this->data;
	}
	return NULL;
}

void MapObject::SetPolygon(LuxPolygon * poly)
{
	this->data_type = 'p';
	if ( poly )
	{
		this->data = (mem_pointer)poly;
		this->has_data = true;
	}
	else
	{
		this->data = NULL;
		this->has_data = false;
	}
}


LuxCanvas * MapObject::GetCanvas()
{
	if (this->data_type == 'M')
	{
		return (LuxCanvas*)this->data;
	}
	return NULL;
}

void MapObject::SetCanvas(LuxCanvas * data)
{
	this->data_type = 'M';
	this->data = (mem_pointer)data;
	this->has_data = true;
}


bool MapObject::CollisionRectangle( LuxRect rect[7] )
{

	LuxSprite * sprite = this->PeekSprite();
	if ( sprite )
	{
		if ( !sprite->has_collision_areas )
		{
			return false;
		}
		for ( uint8_t c = 0; c < 7; c++ )
		{
			rect[c].x = rect[c].y = rect[c].w = rect[c].h = rect[c].z = 0;
			if ( sprite->collisions[c].w )
			{
				//rect[c].x = this->position.x + (sprite->rect.w - (sprite->collisions[c].x + sprite->collisions[c].w)) - this->offset_x;
				if ( this->effects.flip_image&16 )
					rect[c].x = this->position.x + (sprite->sheet_area.w - (sprite->collisions[c].x + sprite->collisions[c].w)) ;
				else
					rect[c].x = this->position.x + sprite->collisions[c].x;
				rect[c].x += this->offset_x;;
				rect[c].y = this->position.y + this->offset_y + sprite->collisions[c].y;
				if ( this->effects.scale_xaxis != 1000 || this->effects.scale_yaxis != 1000 )
				{
					rect[c].w = (sprite->collisions[c].w * this->effects.scale_xaxis) / FRACT_BITS;
					rect[c].h = (sprite->collisions[c].h * this->effects.scale_yaxis) / FRACT_BITS;
				}
				else
				{
					rect[c].w = sprite->collisions[c].w;
					rect[c].h = sprite->collisions[c].h;
				}
			}
		}
		return true;
	}
	return false;
}


std::string MapObject::Info()
{
	std::stringstream s;
	s << this->Name() << "\nl" << (int)this->layer << std::hex << " i" << this->static_map_id;
	return s.str();
}


void MapObject::SetZPos(int32_t z)
{
	if ( z < 0 ) // If Z given is less then 0, z will (layer * 1000) + (y / 8)
	{
		z = (this->layer * 1000) + (this->position.y / 8);
	}
	this->position.z = (int16_t)z;
	this->layer = (uint8_t)(z / 1000);

}

std::string MapObject::Name()
{
	if ( this->type == 'p' )
		return "Polygon";
	else if ( this->type == 'M' )
		return "Canvas";
	else if ( this->type == 'c' )
		return "Circe";
	else if ( this->type == 'r' )
		return "RECTANGLE";
	else if ( this->type == 'l' )
		return "LINE";

	return this->image;
}
