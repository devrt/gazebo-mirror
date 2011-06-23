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
/* Desc: IRSensor proximity sensor
 * Author: Carle Cote
 * Date: 23 february 2004
 * SVN: $Id: IRSensor.hh 4402 2008-03-09 14:39:27Z robotos $
*/

#ifndef IRSENSOR_HH
#define IRSENSOR_HH

#include <vector>

#include "Sensor.hh"
#include "Body.hh"

namespace gazebo
{

  class XMLConfigNode;
  class RayGeom;
  class RaySensor;

/// \addtogroup gazebo_sensor
/// \brief Sensor with one or more rays.
/// \{
/// \defgroup gazebo_ray Ray
/// \brief Sensor with one or more rays.
// \{

/// \brief Sensor with one or more rays.
///
/// This sensor cast rays into the world, tests for intersections, and
/// reports the range to the nearest object.  It is used by ranging
/// sensor models (e.g., sonars and scanning laser range finders).
class IRSensor: public Sensor
{
  /// \brief Constructor
  /// \param body The underlying collision test uses an ODE geom, so
  ///             ray sensors must be attached to a body.
  public: IRSensor(Body *body);

  /// \brief Destructor
  public: virtual ~IRSensor();

  /// Load the ray using parameter from an XMLConfig node
  /// \param node The XMLConfig node
  protected: virtual void LoadChild(XMLConfigNode *node);

  /// Initialize the ray
  protected: virtual void InitChild();

  ///  Update sensed values
  protected: virtual void UpdateChild();
  
  /// Finalize the ray
  protected: virtual void FiniChild();

  /// \brief Get the ray count
  /// \return The number of rays
  public: unsigned int GetIRCount() const;

  /// \brief Get detected range for a ray.
  /// \returns Returns DBL_MAX for no detection.
  public: double GetRange(unsigned int index) const;

  public: Pose GetPose(unsigned int index) const;

  private: std::vector<RaySensor*> irBeams;

};
/// \}
/// \}
}

#endif
