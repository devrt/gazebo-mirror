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
/*
 * Desc: Pressure sensor plugin
 * Author: Steve Peters
 */
#include "PressurePlugin.hh"

using namespace gazebo;
GZ_REGISTER_SENSOR_PLUGIN(PressurePlugin)

/////////////////////////////////////////////////
PressurePlugin::PressurePlugin() : SensorPlugin()
{
}

/////////////////////////////////////////////////
PressurePlugin::~PressurePlugin()
{
}

/////////////////////////////////////////////////
void PressurePlugin::Load(sensors::SensorPtr _sensor, sdf::ElementPtr /*_sdf*/)
{
  // Get the parent sensor.
  this->parentSensor =
    boost::dynamic_pointer_cast<sensors::ContactSensor>(_sensor);

  // Make sure the parent sensor is valid.
  if (!this->parentSensor)
  {
    gzerr << "PressurePlugin requires a ContactSensor.\n";
    return;
  }

  // Connect to the sensor update event.
  this->updateConnection = this->parentSensor->ConnectUpdated(
      boost::bind(&PressurePlugin::OnUpdate, this));

  // Make sure the parent sensor is active.
  this->parentSensor->SetActive(true);

  // Get world name.
  this->worldName = this->parentSensor->GetWorldName();

  // Get name of parent sensor.
  this->parentSensorName = this->parentSensor->GetName();
}

/////////////////////////////////////////////////
void PressurePlugin::Init()
{
  this->node.reset(new transport::Node());
  this->node->Init(this->worldName);

  if (!this->parentSensorName.empty())
  {
    // Create publisher for tactile messages
    this->tactilePub = this->node->Advertise<msgs::Tactile>("~/asdf");
  }
}

/////////////////////////////////////////////////
void PressurePlugin::OnUpdate()
{
  // Get all the contacts.
  msgs::Contacts contacts;
  contacts = this->parentSensor->GetContacts();
  msgs::Tactile tactileMsg;
  for (int i = 0; i < contacts.contact_size(); ++i)
  {
    //std::cout << "Collision between[" << contacts.contact(i).collision1()
    //          << "] and [" << contacts.contact(i).collision2() << "]\n";
    tactileMsg.add_collision_name(contacts.contact(i).collision1());
    tactileMsg.add_collision_id(0);
    double normalForceSum = 0, normalForce;

    for (int j = 0; j < contacts.contact(i).position_size(); ++j)
    {
      //std::cout << j << "  Position:"
      //          << contacts.contact(i).position(j).x() << " "
      //          << contacts.contact(i).position(j).y() << " "
      //          << contacts.contact(i).position(j).z() << "\n";
      //std::cout << "   Normal:"
      //          << contacts.contact(i).normal(j).x() << " "
      //          << contacts.contact(i).normal(j).y() << " "
      //          << contacts.contact(i).normal(j).z() << "\n";
      //std::cout << "   Depth:" << contacts.contact(i).depth(j) << "\n";
      normalForce = contacts.contact(i).normal(j).x() *
                   contacts.contact(i).wrench(j).body_1_force().x() +
                   contacts.contact(i).normal(j).y() *
                   contacts.contact(i).wrench(j).body_1_force().y() +
                   contacts.contact(i).normal(j).z() *
                   contacts.contact(i).wrench(j).body_1_force().z();
      //std::cout << "   Normal force 1: "
      //          << normalForce
      //          << "\n";
      normalForceSum += normalForce;
    }
    double area = 1.0;
    tactileMsg.add_pressure(normalForceSum / area);
  }

  tactileMsg.mutable_time()->set_sec(contacts.time().sec());
  tactileMsg.mutable_time()->set_nsec(contacts.time().nsec());

  if (this->tactilePub)
    this->tactilePub->Publish(tactileMsg);
}
