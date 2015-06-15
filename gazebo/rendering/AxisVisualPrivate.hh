/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
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

#ifndef _AXISVISUAL_PRIVATE_HH_
#define _AXISVISUAL_PRIVATE_HH_

#include "gazebo/rendering/RenderTypes.hh"
#include "gazebo/rendering/VisualPrivate.hh"

namespace gazebo
{
  namespace rendering
  {
    /// \brief Private data for the Axis Visual class.
    class AxisVisualPrivate : public VisualPrivate
    {
      /// \brief Pointer to the x-axis visual.
      public: ArrowVisualPtr xAxis;

      /// \brief Pointer to the y-axis visual.
      public: ArrowVisualPtr yAxis;

      /// \brief Pointer to the z-axis visual.
      public: ArrowVisualPtr zAxis;

      /// \brief Parent element name.
      public: std::string elementName;

      /// \brief Scale based on the size of the parent element.
      public: math::Vector3 scaleToElement;
    };
  }
}
#endif
