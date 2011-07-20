/*
 * Copyright 2011 Nate Koenig & Andrew Howard
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
/* Desc: Base class for all sensors
 * Author: Nathan Koenig
 * Date: 25 May 2007
 */

#ifndef SENSOR_HH
#define SENSOR_HH

#include <vector>

#include "sdf/sdf.h"

#include "common/Events.hh"
#include "common/Time.hh"
#include "math/Pose.hh"

namespace gazebo
{
  namespace sensors
  {
    /// \addtogroup gazebo_sensors
    /// \{
    
    /// \brief Base class for sensors
    class Sensor
    {
      /// \brief  Constructor
      public: Sensor();
    
      /// \brief  Destructor
      public: virtual ~Sensor();
    
      /// \brief Load the sensor with SDF parameters
      /// \param _sdf SDF Sensor parameteres
      public: virtual void Load( sdf::ElementPtr &_sdf );

      /// \brief Load the sensor with default parameters
      public: virtual void Load();
  
      /// \brief  Initialize the sensor
      public: virtual void Init();
    
      /// \brief  Update the sensor
      public: virtual void Update(bool force);
    
      /// \brief  Finalize the sensor
      public: virtual void Fini();

      /// \brief Get name 
      public: std::string GetName() const;

      /// \brief Get the type of the sensor
      public: std::string GetSensorType(){return typeName;}
  
      /// \brief Get the current pose
      public: virtual math::Pose GetPose() const;
  
      /// \brief Set whether the sensor is active or not
      public: virtual void SetActive(bool value);

      public: bool IsActive();
    
      /// \brief Load a plugin for this sensor
      /// \param _sdf SDF parameters
      private: void LoadPlugin( sdf::ElementPtr &_sdf );
    
      /// \brief True if active
      protected: bool active;
      protected: sdf::ElementPtr sdf; 
      protected: math::Pose pose;
      protected: std::string typeName;
      protected: std::vector<event::ConnectionPtr> connections;
    };
    /// \}
  }
}
#endif
