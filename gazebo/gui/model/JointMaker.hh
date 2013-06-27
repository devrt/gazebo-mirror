/*
 * Copyright 2012 Open Source Robotics Foundation
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

#ifndef _JOINTMAKER_HH_
#define _JOINTMAKER_HH_

#include <string>
#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/RenderTypes.hh"

namespace ogre
{
  class SceneNode;
}

namespace gazebo
{
  namespace gui
  {
    /// \addtogroup gazebo_gui
    /// \{

    /// \class JointMaker JointMaker.hh
    /// \brief Joint visualization
    class JointMaker : public rendering::Visual
    {
      /// \brief Constructor
      /// \param[in] _name Name of the joint visual
      /// \param[in] _vis Pointer to the parent visual
      public: JointMaker(const std::string &_name,
          rendering::VisualPtr _vis);

      /// \brief Destructor
      public: virtual ~JointMaker();

      /// \brief Set parent of joint.
      /// \param[in] _parent Pointer to parent visual.
      /// \param[in] _offset Offset relative to parent origin where the joint
      /// is to be attached to.
      public: void SetParent(rendering::VisualPtr _parent,
          math::Vector3 _offset = math::Vector3::Zero);

      /// \brief Set child of joint.
      /// \param[in] _child Pointer to child visual.
      /// \param[in] _offset Offset relative to child origin where the joint
      /// is to be attached to.
      public: void SetChild(rendering::VisualPtr _child,
          math::Vector3 _offset = math::Vector3::Zero);

      /// \brief Visual line used to represent joint connecting parent and child
      private: rendering::DynamicLines *jointLine;
    };
    /// \}
  }
}
#endif
