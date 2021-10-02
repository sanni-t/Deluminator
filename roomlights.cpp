#include "roomlights.h"

RoomLights::RoomLights(LightInfo* light_info_array, uint8_t num_of_lights)
{
    num_lights = num_of_lights;
    _lights = new LightInfo[num_of_lights];
    for(int i=0; i<num_of_lights; i++)
    {
        _lights[i] = light_info_array[i];
    }
}

LightInfo* RoomLights::all_lights()
{
    return _lights;
}

LightInfo* RoomLights::get_next_light()
{
    if(_last_handled_light >= num_lights-1)
    {
        _last_handled_light = 0;
    }
    else
    {
        _last_handled_light++;
    }
    return &(_lights[_last_handled_light]);
}

void RoomLights::update_all_lights_status(LightStatus status)
{
    for(int i = 0; i < num_lights; i++)
    {
        _lights[i].status = status;
    }
    _last_handled_light = 0;
}

bool RoomLights::are_all_off()
{

    for(int i = 0; i < num_lights; i++)
    {
        if(_lights[i].status == LightStatus::ON) 
        {
            return false;
        }
    }
    return true;
}

bool RoomLights::are_all_on()
{

    for(int i = 0; i < num_lights; i++)
    {
        if(_lights[i].status == LightStatus::OFF) 
        {
            return false;
        }
    }
    return true;
}

RoomLights::~RoomLights()
{
    if (_lights)
    {
        delete[] _lights;
    }
}