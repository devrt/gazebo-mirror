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

#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wshadow"

#include <gtest/gtest.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#include <map>
#include <string>

#include "gazebo/transport/transport.hh"

#include "gazebo/common/SystemPaths.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/physics/World.hh"
#include "gazebo/physics/PhysicsTypes.hh"
#include "gazebo/physics/Physics.hh"
#include "gazebo/sensors/sensors.hh"
#include "gazebo/rendering/rendering.hh"
#include "gazebo/msgs/msgs.hh"

#include "gazebo_config.h"
#include "gazebo/Server.hh"

#include "test_config.h"

using namespace gazebo;

class ServerFixture : public testing::Test
{
  protected: ServerFixture()
             {
               this->server = NULL;
               this->serverRunning = false;
               this->paused = false;
               this->percentRealTime = 0;
               this->gotImage = 0;
               this->imgData = NULL;
               this->serverThread = NULL;

               common::Console::Instance()->Init("test.log");
               common::SystemPaths::Instance()->AddGazeboPaths(
                   TEST_REGRESSION_PATH);

               // Add local search paths
               std::string path = TEST_REGRESSION_PATH;
               path += "/../..";
               gazebo::common::SystemPaths::Instance()->AddGazeboPaths(path);

               path = TEST_REGRESSION_PATH;
               path += "/../../sdf";
               gazebo::common::SystemPaths::Instance()->AddGazeboPaths(path);

               path = TEST_REGRESSION_PATH;
               path += "/../../gazebo";
               gazebo::common::SystemPaths::Instance()->AddGazeboPaths(path);

               path = TEST_REGRESSION_PATH;
               path += "/../../build/plugins";
               gazebo::common::SystemPaths::Instance()->AddPluginPaths(path);
             }

  protected: virtual void TearDown()
             {
               this->Unload();
             }

  protected: virtual void Unload()
             {
               this->serverRunning = false;
               if (this->node)
                 this->node->Fini();

               if (this->server)
               {
                 this->server->Stop();

                 if (this->serverThread)
                 {
                   this->serverThread->join();
                 }
               }

               delete this->serverThread;
               this->serverThread = NULL;
             }

  protected: virtual void Load(const std::string &_worldFilename)
             {
               this->Load(_worldFilename, false);
             }

  protected: virtual void Load(const std::string &_worldFilename, bool _paused)
             {
               this->Load(_worldFilename, _paused, "");
             }

  protected: virtual void Load(const std::string &_worldFilename,
                               bool _paused, const std::string &_physics)
             {
               delete this->server;
               this->server = NULL;

               // Create, load, and run the server in its own thread
               this->serverThread = new boost::thread(
                  boost::bind(&ServerFixture::RunServer, this, _worldFilename,
                              _paused, _physics));

               // Wait for the server to come up
               // Use a 30 second timeout.
               int waitCount = 0, maxWaitCount = 3000;
               while ((!this->server || !this->server->GetInitialized()) &&
                      ++waitCount < maxWaitCount)
                 common::Time::MSleep(10);
               ASSERT_LT(waitCount, maxWaitCount);

               this->node = transport::NodePtr(new transport::Node());
               ASSERT_NO_THROW(this->node->Init());
               this->poseSub = this->node->Subscribe("~/pose/info",
                   &ServerFixture::OnPose, this, true);
               this->statsSub = this->node->Subscribe("~/world_stats",
                   &ServerFixture::OnStats, this);

               this->factoryPub =
                 this->node->Advertise<msgs::Factory>("~/factory");
               this->factoryPub->WaitForConnection();

               // Wait for the world to reach the correct pause state.
               // This might not work properly with multiple worlds.
               // Use a 30 second timeout.
               waitCount = 0;
               maxWaitCount = 3000;
               while ((!physics::get_world() ||
                        physics::get_world()->IsPaused() != _paused) &&
                      ++waitCount < maxWaitCount)
                 common::Time::MSleep(10);
               ASSERT_LT(waitCount, maxWaitCount);

             }

  protected: void RunServer(const std::string &_worldFilename)
             {
               this->RunServer(_worldFilename, false, "");
             }

  protected: rendering::ScenePtr GetScene(
                 const std::string &_sceneName = "default")
             {
               // Wait for the scene to get loaded.
               int i = 0;
               while (rendering::get_scene(_sceneName) == NULL && i < 20)
               {
                 common::Time::MSleep(100);
                 ++i;
               }

               if (i >= 20)
                 gzerr << "Unable to load the rendering scene.\n"
                   << "Test will fail";

               EXPECT_LT(i, 20);
               return rendering::get_scene(_sceneName);
             }

  protected: void RunServer(const std::string &_worldFilename, bool _paused,
                            const std::string &_physics)
             {
               ASSERT_NO_THROW(this->server = new Server());
               if (_physics.length())
                 ASSERT_NO_THROW(this->server->LoadFile(_worldFilename,
                                                        _physics));
               else
                 ASSERT_NO_THROW(this->server->LoadFile(_worldFilename));
               ASSERT_NO_THROW(this->server->Init());

               rendering::create_scene(
                   gazebo::physics::get_world()->GetName(), false);

               this->SetPause(_paused);

               this->server->Run();

               rendering::remove_scene(gazebo::physics::get_world()->GetName());

               ASSERT_NO_THROW(this->server->Fini());
               delete this->server;
               this->server = NULL;
             }

  protected: void OnStats(ConstWorldStatisticsPtr &_msg)
             {
               this->simTime = msgs::Convert(_msg->sim_time());
               this->realTime = msgs::Convert(_msg->real_time());
               this->pauseTime = msgs::Convert(_msg->pause_time());
               this->paused = _msg->paused();

               if (this->realTime == 0)
                 this->percentRealTime = 0;
               else
                 this->percentRealTime =
                   (this->simTime / this->realTime).Double();

               this->serverRunning = true;
             }

  protected: void SetPause(bool _pause)
             {
               physics::pause_worlds(_pause);
             }

  protected: double GetPercentRealTime() const
             {
               while (!this->serverRunning)
                 common::Time::MSleep(100);

               return this->percentRealTime;
             }

  protected: void OnPose(ConstPose_VPtr &_msg)
             {
               boost::mutex::scoped_lock lock(this->receiveMutex);
               for (int i = 0; i < _msg->pose_size(); ++i)
               {
                 this->poses[_msg->pose(i).name()] =
                   msgs::Convert(_msg->pose(i));
               }
             }

  protected: math::Pose GetEntityPose(const std::string &_name)
             {
               boost::mutex::scoped_lock lock(this->receiveMutex);

               std::map<std::string, math::Pose>::iterator iter;
               iter = this->poses.find(_name);
               EXPECT_TRUE(iter != this->poses.end());
               return iter->second;
             }

  protected: bool HasEntity(const std::string &_name)
             {
               boost::mutex::scoped_lock lock(this->receiveMutex);
               std::map<std::string, math::Pose>::iterator iter;
               iter = this->poses.find(_name);
               return iter != this->poses.end();
             }

  protected: void PrintImage(const std::string &_name, unsigned char **_image,
                 unsigned int _width, unsigned int _height, unsigned int _depth)
             {
               unsigned int count = _height * _width * _depth;
               printf("\n");
               printf("static unsigned char __%s[] = {", _name.c_str());
               unsigned int i;
               for (i = 0; i < count-1; i++)
               {
                 if (i % 10 == 0)
                   printf("\n");
                 else
                   printf(" ");
                 printf("%d,", (*_image)[i]);
               }
               printf(" %d};\n", (*_image)[i]);
               printf("static unsigned char *%s = __%s;\n", _name.c_str(),
                   _name.c_str());
             }

  protected: void PrintScan(const std::string &_name, double *_scan,
                            unsigned int _sampleCount)
             {
               printf("static double __%s[] = {\n", _name.c_str());
               for (unsigned int i = 0; i < _sampleCount-1; ++i)
               {
                 if ((i+1) % 5 == 0)
                   printf("%13.10f,\n", math::precision(_scan[i], 10));
                 else
                   printf("%13.10f, ", math::precision(_scan[i], 10));
               }
               printf("%13.10f};\n",
                   math::precision(_scan[_sampleCount-1], 10));
               printf("static double *%s = __%s;\n", _name.c_str(),
                   _name.c_str());
             }

  protected: void FloatCompare(float *_scanA, float *_scanB,
                 unsigned int _sampleCount, float &_diffMax,
                 float &_diffSum, float &_diffAvg)
             {
               float diff;
               _diffMax = 0;
               _diffSum = 0;
               _diffAvg = 0;
               for (unsigned int i = 0; i < _sampleCount; ++i)
               {
                 diff = fabs(math::precision(_scanA[i], 10) -
                             math::precision(_scanB[i], 10));
                 _diffSum += diff;
                 if (diff > _diffMax)
                 {
                   _diffMax = diff;
                 }
               }
               _diffAvg = _diffSum / _sampleCount;
             }

  protected: void DoubleCompare(double *_scanA, double *_scanB,
                 unsigned int _sampleCount, double &_diffMax,
                 double &_diffSum, double &_diffAvg)
             {
               double diff;
               _diffMax = 0;
               _diffSum = 0;
               _diffAvg = 0;
               for (unsigned int i = 0; i < _sampleCount; ++i)
               {
                 diff = fabs(math::precision(_scanA[i], 10) -
                             math::precision(_scanB[i], 10));
                 _diffSum += diff;
                 if (diff > _diffMax)
                 {
                   _diffMax = diff;
                 }
               }
               _diffAvg = _diffSum / _sampleCount;
             }

  protected: void ImageCompare(unsigned char **_imageA,
                 unsigned char *_imageB[],
                 unsigned int _width, unsigned int _height, unsigned int _depth,
                 unsigned int &_diffMax, unsigned int &_diffSum,
                 double &_diffAvg)
             {
               _diffMax = 0;
               _diffSum = 0;
               _diffAvg = 0;

               for (unsigned int y = 0; y < _height; y++)
               {
                 for (unsigned int x = 0; x < _width*_depth; x++)
                 {
                   unsigned int a = (*_imageA)[(y*_width*_depth)+x];
                   unsigned int b = (*_imageB)[(y*_width*_depth)+x];

                   unsigned int diff = (unsigned int)(fabs(a - b));

                   if (diff > _diffMax)
                     _diffMax = diff;

                   _diffSum += diff;
                 }
               }
               _diffAvg = _diffSum / (_height*_width*_depth);
             }

  private: void OnNewFrame(const unsigned char *_image,
                              unsigned int _width, unsigned int _height,
                              unsigned int _depth,
                              const std::string &/*_format*/)
           {
             memcpy(*this->imgData, _image, _width * _height * _depth);
             this->gotImage+= 1;
           }

  protected: void GetFrame(const std::string &_cameraName,
                 unsigned char **_imgData, unsigned int &_width,
                 unsigned int &_height)
             {
               sensors::SensorPtr sensor = sensors::get_sensor(_cameraName);
               EXPECT_TRUE(sensor);
               sensors::CameraSensorPtr camSensor =
                 boost::shared_dynamic_cast<sensors::CameraSensor>(sensor);

               _width = camSensor->GetImageWidth();
               _height = camSensor->GetImageHeight();

               if (*_imgData)
               {
                 delete *_imgData;
                 *_imgData = NULL;
               }
               (*_imgData) = new unsigned char[_width *_height*3];
               this->imgData = _imgData;

               this->gotImage = 0;
               event::ConnectionPtr c =
                 camSensor->GetCamera()->ConnectNewImageFrame(
                     boost::bind(&ServerFixture::OnNewFrame,
                                 this, _1, _2, _3, _4, _5));

               while (this->gotImage < 20)
                 common::Time::MSleep(10);

               camSensor->GetCamera()->DisconnectNewImageFrame(c);
             }

  protected: void SpawnCamera(const std::string &_modelName,
                 const std::string &_cameraName,
                 const math::Vector3 &_pos, const math::Vector3 &_rpy,
                 unsigned int _width = 320, unsigned int _height = 240,
                 double _rate = 25)
             {
               msgs::Factory msg;
               std::ostringstream newModelStr;

               newModelStr << "<sdf version='" << SDF_VERSION << "'>"
                 << "<model name ='" << _modelName << "'>"
                 << "<static>true</static>"
                 << "<pose>" << _pos.x << " "
                             << _pos.y << " "
                             << _pos.z << " "
                             << _rpy.x << " "
                             << _rpy.y << " "
                             << _rpy.z << "</pose>"
                 << "<link name ='body'>"
                 << "  <sensor name ='" << _cameraName
                 << "' type ='camera'>"
                 << "    <always_on>1</always_on>"
                 << "    <update_rate>" << _rate << "</update_rate>"
                 << "    <visualize>true</visualize>"
                 << "    <camera>"
                 << "      <horizontal_fov>0.78539816339744828</horizontal_fov>"
                 << "      <image>"
                 << "        <width>" << _width << "</width>"
                 << "        <height>" << _height << "</height>"
                 << "        <format>R8G8B8</format>"
                 << "      </image>"
                 << "      <clip>"
                 << "        <near>0.1</near><far>100</far>"
                 << "      </clip>"
                 // << "      <save enabled ='true' path ='/tmp/camera/'/>"
                 << "    </camera>"
                 << "  </sensor>"
                 << "</link>"
                 << "</model>"
                 << "</sdf>";

               msg.set_sdf(newModelStr.str());
               this->factoryPub->Publish(msg);

               int i = 0;
               // Wait for the entity to spawn
               while (!this->HasEntity(_modelName) && i < 50)
               {
                 common::Time::MSleep(20);
                 ++i;
               }
               EXPECT_LT(i, 50);
             }

  protected: void SpawnRaySensor(const std::string &_modelName,
                 const std::string &_raySensorName,
                 const math::Vector3 &_pos, const math::Vector3 &_rpy,
                 double _hMinAngle = -2.0, double _hMaxAngle = 2.0,
                 double _minRange = 0.08, double _maxRange = 10,
                 double _rangeResolution = 0.01, unsigned int _samples = 640)
             {
               msgs::Factory msg;
               std::ostringstream newModelStr;

               newModelStr << "<sdf version='" << SDF_VERSION << "'>"
                 << "<model name ='" << _modelName << "'>"
                 << "<static>true</static>"
                 << "<pose>" << _pos.x << " "
                             << _pos.y << " "
                             << _pos.z << " "
                             << _rpy.x << " "
                             << _rpy.y << " "
                             << _rpy.z << "</pose>"
                 << "<link name ='body'>"
                 << "<collision name='parent_collision'>"
                 << "  <pose>0 0 0.0205 0 0 0</pose>"
                 << "  <geometry>"
                 << "    <cylinder>"
                 << "      <radius>0.021</radius>"
                 << "      <length>0.029</length>"
                 << "    </cylinder>"
                 << "  </geometry>"
                 << "</collision>"
                 << "  <sensor name ='" << _raySensorName << "' type ='ray'>"
                 << "    <ray>"
                 << "      <scan>"
                 << "        <horizontal>"
                 << "          <samples>" << _samples << "</samples>"
                 << "          <resolution> 1 </resolution>"
                 << "          <min_angle>" << _hMinAngle << "</min_angle>"
                 << "          <max_angle>" << _hMaxAngle << "</max_angle>"
                 << "        </horizontal>"
                 << "      </scan>"
                 << "      <range>"
                 << "        <min>" << _minRange << "</min>"
                 << "        <max>" << _maxRange << "</max>"
                 << "        <resolution>" << _rangeResolution <<"</resolution>"
                 << "      </range>"
                 << "    </ray>"
                 << "  </sensor>"
                 << "</link>"
                 << "</model>"
                 << "</sdf>";

               msg.set_sdf(newModelStr.str());
               this->factoryPub->Publish(msg);

               int i = 0;
               // Wait for the entity to spawn
               while (!this->HasEntity(_modelName) && i < 50)
               {
                 common::Time::MSleep(20);
                 ++i;
               }
               EXPECT_LT(i, 50);
             }

  protected: void SpawnCylinder(const std::string &_name,
                 const math::Vector3 &_pos, const math::Vector3 &_rpy)
             {
               msgs::Factory msg;
               std::ostringstream newModelStr;

               newModelStr << "<sdf version='" << SDF_VERSION << "'>"
                 << "<model name ='" << _name << "'>"
                 << "<pose>" << _pos.x << " "
                             << _pos.y << " "
                             << _pos.z << " "
                             << _rpy.x << " "
                             << _rpy.y << " "
                             << _rpy.z << "</pose>"
                 << "<link name ='body'>"
                 << "  <collision name ='geom'>"
                 << "    <geometry>"
                 << "      <cylinder>"
                 << "        <radius>.5</radius><length>1.0</length>"
                 << "      </cylinder>"
                 << "    </geometry>"
                 << "  </collision>"
                 << "  <visual name ='visual'>"
                 << "    <geometry>"
                 << "      <cylinder>"
                 << "        <radius>.5</radius><length>1.0</length>"
                 << "      </cylinder>"
                 << "    </geometry>"
                 << "  </visual>"
                 << "</link>"
                 << "</model>"
                 << "</sdf>";

               msg.set_sdf(newModelStr.str());
               this->factoryPub->Publish(msg);

               // Wait for the entity to spawn
               while (!this->HasEntity(_name))
                 common::Time::MSleep(10);
             }


  protected: void SpawnSphere(const std::string &_name,
                 const math::Vector3 &_pos, const math::Vector3 &_rpy,
                 bool _wait = true)
             {
               msgs::Factory msg;
               std::ostringstream newModelStr;

               newModelStr << "<sdf version='" << SDF_VERSION << "'>"
                 << "<model name ='" << _name << "'>"
                 << "<pose>" << _pos.x << " "
                             << _pos.y << " "
                             << _pos.z << " "
                             << _rpy.x << " "
                             << _rpy.y << " "
                             << _rpy.z << "</pose>"
                 << "<link name ='body'>"
                 << "  <collision name ='geom'>"
                 << "    <geometry>"
                 << "      <sphere><radius>.5</radius></sphere>"
                 << "    </geometry>"
                 << "  </collision>"
                 << "  <visual name ='visual'>"
                 << "    <geometry>"
                 << "      <sphere><radius>.5</radius></sphere>"
                 << "    </geometry>"
                 << "  </visual>"
                 << "</link>"
                 << "</model>"
                 << "</sdf>";

               msg.set_sdf(newModelStr.str());
               this->factoryPub->Publish(msg);

               // Wait for the entity to spawn
               while (_wait && !this->HasEntity(_name))
                 common::Time::MSleep(10);
             }

  protected: void SpawnSphere(const std::string &_name,
                 const math::Vector3 &_pos, const math::Vector3 &_rpy,
                 const math::Vector3 &_cog, double _radius,
                 bool _wait = true)
             {
               msgs::Factory msg;
               std::ostringstream newModelStr;

               newModelStr << "<sdf version='" << SDF_VERSION << "'>"
                 << "<model name ='" << _name << "'>"
                 << "<pose>" << _pos.x << " "
                             << _pos.y << " "
                             << _pos.z << " "
                             << _rpy.x << " "
                             << _rpy.y << " "
                             << _rpy.z << "</pose>"
                 << "<link name ='body'>"
                 << "  <inertial>"
                 << "    <pose>" << _cog << " 0 0 0</pose>"
                 << "  </inertial>"
                 << "  <collision name ='geom'>"
                 << "    <geometry>"
                 << "      <sphere><radius>" << _radius << "</radius></sphere>"
                 << "    </geometry>"
                 << "  </collision>"
                 << "  <visual name ='visual'>"
                 << "    <geometry>"
                 << "      <sphere><radius>" << _radius << "</radius></sphere>"
                 << "    </geometry>"
                 << "  </visual>"
                 << "</link>"
                 << "</model>"
                 << "</sdf>";

               msg.set_sdf(newModelStr.str());
               this->factoryPub->Publish(msg);

               // Wait for the entity to spawn
               while (_wait && !this->HasEntity(_name))
                 common::Time::MSleep(10);
             }

  protected: void SpawnBox(const std::string &_name,
                 const math::Vector3 &_size, const math::Vector3 &_pos,
                 const math::Vector3 &_rpy)
             {
               msgs::Factory msg;
               std::ostringstream newModelStr;

               newModelStr << "<sdf version='" << SDF_VERSION << "'>"
                 << "<model name ='" << _name << "'>"
                 << "<pose>" << _pos.x << " "
                             << _pos.y << " "
                             << _pos.z << " "
                             << _rpy.x << " "
                             << _rpy.y << " "
                             << _rpy.z << "</pose>"
                 << "<link name ='body'>"
                 << "  <collision name ='geom'>"
                 << "    <geometry>"
                 << "      <box><size>" << _size << "</size></box>"
                 << "    </geometry>"
                 << "  </collision>"
                 << "  <visual name ='visual'>"
                 << "    <geometry>"
                 << "      <box><size>" << _size << "</size></box>"
                 << "    </geometry>"
                 << "  </visual>"
                 << "</link>"
                 << "</model>"
                 << "</sdf>";

               msg.set_sdf(newModelStr.str());
               this->factoryPub->Publish(msg);

               // Wait for the entity to spawn
               while (!this->HasEntity(_name))
                 common::Time::MSleep(10);
             }

  protected: void SpawnEmptyLink(const std::string &_name,
                 const math::Vector3 &_pos,
                 const math::Vector3 &_rpy)
             {
               msgs::Factory msg;
               std::ostringstream newModelStr;

               newModelStr << "<sdf version='" << SDF_VERSION << "'>"
                 << "<model name ='" << _name << "'>"
                 << "<pose>" << _pos.x << " "
                             << _pos.y << " "
                             << _pos.z << " "
                             << _rpy.x << " "
                             << _rpy.y << " "
                             << _rpy.z << "</pose>"
                 << "<link name ='body'>"
                 << "</link>"
                 << "</model>"
                 << "</sdf>";

               msg.set_sdf(newModelStr.str());
               this->factoryPub->Publish(msg);

               // Wait for the entity to spawn
               while (!this->HasEntity(_name))
                 common::Time::MSleep(10);
             }

  protected: void SpawnModel(const std::string &_filename)
             {
               msgs::Factory msg;
               msg.set_sdf_filename(_filename);
               this->factoryPub->Publish(msg);
             }

  protected: void SpawnSDF(const std::string &_sdf)
             {
               // Wait for the first pose message
               while (this->poses.size() == 0)
                 common::Time::MSleep(10);

               msgs::Factory msg;
               msg.set_sdf(_sdf);
               this->factoryPub->Publish(msg);
             }

  protected: void LoadPlugin(const std::string &_filename,
                             const std::string &_name)
             {
               // Get the first world...we assume it the only one running
               physics::WorldPtr world = physics::get_world();
               world->LoadPlugin(_filename, _name, sdf::ElementPtr());
             }

  protected: physics::ModelPtr GetModel(const std::string &_name)
             {
               // Get the first world...we assume it the only one running
               physics::WorldPtr world = physics::get_world();
               return world->GetModel(_name);
             }


  protected: void RemovePlugin(const std::string &_name)
             {
               // Get the first world...we assume it the only one running
               physics::WorldPtr world = physics::get_world();
               world->RemovePlugin(_name);
             }

  protected: Server *server;
  protected: boost::thread *serverThread;

  protected: transport::NodePtr node;
  protected: transport::SubscriberPtr poseSub;
  protected: transport::SubscriberPtr statsSub;
  protected: transport::PublisherPtr factoryPub;

  protected: std::map<std::string, math::Pose> poses;
  protected: boost::mutex receiveMutex;

  private: unsigned char **imgData;
  private: int gotImage;

  protected: common::Time simTime, realTime, pauseTime;
  private: double percentRealTime;
  private: bool paused;
  private: bool serverRunning;
};
