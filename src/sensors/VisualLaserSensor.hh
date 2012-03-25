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
/* Desc: RaySensor proximity sensor
 * Author: Nate Koenig
 * Date: 23 february 2004
*/

#ifndef VISUALLASERSENSOR_HH
#define VISUALLASERSENSOR_HH

#include <vector>

#include "math/Angle.hh"
#include "math/Pose.hh"
#include "transport/TransportTypes.hh"
#include "sensors/Sensor.hh"
#include "rendering/RenderTypes.hh"

namespace gazebo
{
  class OgreDynamicLines;
  class Collision;
  class MultiRayShape;

  namespace sensors
  {
    /// \addtogroup gazebo_sensors
    /// \{
    /// \brief Sensor with one or more rays.
    ///
    /// This sensor cast rays into the world, tests for intersections, and
    /// reports the range to the nearest object.  It is used by ranging
    /// sensor models (e.g., sonars and scanning laser range finders).
    class VisualLaserSensor: public Sensor
    {
      /// \brief Constructor
      public: VisualLaserSensor();

      /// \brief Destructor
      public: virtual ~VisualLaserSensor();

      /// Load the ray using parameter from an SDF
      /// \param node The XMLConfig node
      public: virtual void Load(const std::string &_worldName,
                                sdf::ElementPtr &_sdf);

      public: virtual void Load(const std::string &_worldName);

      /// Initialize the ray
      public: virtual void Init();

      /// \brief Update the sensor information
      protected: virtual void UpdateImpl(bool _force);

      /// Finalize the ray
      protected: virtual void Fini();

      public: rendering::VisualLaserPtr GetLaserCamera() const
              {return this->laserCam;}

      /// \brief Get the minimum angle
      /// \return The minimum angle
      public: math::Angle GetAngleMin() const;

      /// \brief Get the maximum angle
      /// \return the maximum angle
      public: math::Angle GetAngleMax() const;

      /// \brief Get radians between each range
      public: double GetAngleResolution() const;

      /// \brief Get the minimum range
      /// \return The minimum range
      public: double GetRangeMin() const;

      /// \brief Get the maximum range
      /// \return The maximum range
      public: double GetRangeMax() const;

      /// \brief Get the range resolution
      public: double GetRangeResolution() const;

      /// \brief Get the ray count
      /// \return The number of rays
      public: int GetRayCount() const;

      /// \brief Get the range count
      /// \return The number of ranges
      public: int GetRangeCount() const;

      /// \brief Get the vertical scan line count
      /// \return The number of scan lines vertically
      public: int GetVerticalRayCount() const;

      /// \brief Get the vertical scan line count
      /// \return The number of scan lines vertically
      public: int GetVerticalRangeCount() const;

      /// \brief Get the vertical scan bottom angle
      /// \return The minimum angle of the scan block
      public: math::Angle GetVerticalAngleMin() const;

      /// \brief Get the vertical scan line top angle
      /// \return The Maximum angle of the scan block
      public: math::Angle GetVerticalAngleMax() const;

      /// \brief Get detected range for a ray.
      ///         Warning: If you are accessing all the ray data in a loop
      ///         it's possible that the Ray will update in the middle of
      ///         your aceess loop. This means some data will come from one
      ///         scan, and some from another scan. You can solve this
      ///         problem by using SetActive(false) <your accessor loop>
      ///         SetActive(true).
      /// \return Returns DBL_MAX for no detection.
      public: double GetRange(int index);

      /// \brief Get all the ranges
      /// \param _range A vector that will contain all the range data
      public: void GetRanges(std::vector<double> &_ranges);

      /// \brief Get detected retro (intensity) value for a ray.
      ///         Warning: If you are accessing all the ray data in a loop
      ///         it's possible that the Ray will update in the middle of
      ///         your aceess loop. This means some data will come from one
      ///         scan, and some from another scan. You can solve this
      ///         problem by using SetActive(false) <your accessor loop>
      ///         SetActive(true).
      public: double GetRetro(int index);

      /// \brief Get detected fiducial value for a ray.
      ///         Warning: If you are accessing all the ray data in a loop
      ///         it's possible that the Ray will update in the middle of
      ///         your aceess loop. This means some data will come from one
      ///         scan, and some from another scan. You can solve this
      ///         problem by using SetActive(false) <your accessor loop>
      ///         SetActive(true).
      public: int GetFiducial(int index);

      public: unsigned int GetCameraCount();

      public: double Get1stRatio();

      public: double Get2ndRatio();
      
      public: double GetHFOV();
      
      public: double GetCHFOV();

      public: double GetVFOV();

      public: double GetHAngle();

      public: double GetVAngle();
      
      private: void OnPose(ConstPosePtr &_msg);

      protected: math::Vector3 offset;
      protected: sdf::ElementPtr rayElem;
      protected: sdf::ElementPtr scanElem;
      protected: sdf::ElementPtr horzElem;
      protected: sdf::ElementPtr vertElem;
      protected: sdf::ElementPtr rangeElem;
      protected: sdf::ElementPtr cameraElem;

      protected: unsigned int cameraCount;

      protected: double hfov, vfov, chfov, hang, vang;
      protected: double near, far;
      protected: unsigned int width_1st, height_1st;
      protected: unsigned int width_2nd, height_2nd;
      protected: double ratio_1st, ratio_2nd;
      
      private: rendering::VisualLaserPtr laserCam;
      private: rendering::ScenePtr scene;

      private: transport::NodePtr node;
      private: transport::PublisherPtr scanPub;
      private: boost::mutex *mutex;
      private: msgs::LaserScan laserMsg;
    };
    /// \}
  }
}

#endif
