/*
 * Copyright (C) 2012-2013 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef _GAZEBO_GPU_LASER_PLUGIN_HH_
#define _GAZEBO_GPU_LASER_PLUGIN_HH_

#include <string>

#include "gazebo/common/Plugin.hh"
#include "gazebo/sensors/CameraSensor.hh"
#include "gazebo/rendering/RenderTypes.hh"
#include "gazebo/gazebo.hh"

namespace gazebo
{
  class GpuRayPlugin : public SensorPlugin
  {
    public: GpuRayPlugin();

    public: void Load(sensors::SensorPtr _sensor, sdf::ElementPtr _sdf);

    public: virtual void OnNewLaserFrame(const float *_image,
                unsigned int _width, unsigned int _height,
                unsigned int _depth, const std::string &_format);

    protected: unsigned int width, height/*, depth*/;

    protected: sensors::GpuRaySensorPtr parentSensor;

    private: event::ConnectionPtr newLaserFrameConnection;
  };
}
#endif
