/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003  
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* Desc: Simulation Interface for Player
 * Author: Nate Koenig
 * Date: 2 March 2006
 * CVS: $Id$
 */

#include "gazebo.h"
#include "GazeboError.hh"
#include "GazeboDriver.hh"
#include "SimulationInterface.hh"

using namespace gazebo;

///////////////////////////////////////////////////////////////////////////////
// Constructor
SimulationInterface::SimulationInterface(player_devaddr_t addr, GazeboDriver *driver, ConfigFile *cf, int section)
  : GazeboInterface(addr, driver, cf, section)
{
  // Get the ID of the interface
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GazeboClient::prefixId);
  strcat(this->gz_id, cf->ReadString(section, "server_id", "default"));

  // ID of the server
  int serverId = atoi((char*)cf->ReadString(section,"server_id","default"));

  // Initialize the Client. Creates the SHM connection
  GazeboClient::Init(serverId, "");

  this->iface = new SimulationIface();
}

///////////////////////////////////////////////////////////////////////////////
// Destructor
SimulationInterface::~SimulationInterface()
{
  delete this->iface;
}

///////////////////////////////////////////////////////////////////////////////
// Handle all messages. This is called from GazeboDriver
int SimulationInterface::ProcessMessage(QueuePointer &respQueue,
                   player_msghdr_t *hdr, void *data)
{
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
        PLAYER_SIMULATION_REQ_SET_POSE3D, this->device_addr))
  {
    player_simulation_pose3d_req_t *req = 
      (player_simulation_pose3d_req_t*)(data);

    this->iface->Lock(1);

    strcpy((char*)this->iface->data->model_name,req->name);
    strcpy((char*)this->iface->data->model_req,"set_pose3d");

    this->iface->data->model_pose.x = req->pose.px;
    this->iface->data->model_pose.y = req->pose.py;
    this->iface->data->model_pose.z = req->pose.pz;

    this->iface->data->model_pose.roll = req->pose.proll;
    this->iface->data->model_pose.pitch = req->pose.ppitch;
    this->iface->data->model_pose.yaw = req->pose.pyaw;
    this->iface->Unlock();

    this->driver->Publish(this->device_addr, respQueue,
        PLAYER_MSGTYPE_RESP_ACK, PLAYER_SIMULATION_REQ_SET_POSE3D);

  }
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
        PLAYER_SIMULATION_REQ_SET_POSE2D, this->device_addr))
  {
    player_simulation_pose2d_req_t *req = 
      (player_simulation_pose2d_req_t*)(data);

    this->iface->Lock(1);

    strcpy((char*)this->iface->data->model_name,req->name);
    strcpy((char*)this->iface->data->model_req,"set_pose2d");

    this->iface->data->model_pose.x = req->pose.px;
    this->iface->data->model_pose.y = req->pose.py;
    this->iface->data->model_pose.yaw = req->pose.pa;
    this->iface->Unlock();

    this->driver->Publish(this->device_addr, respQueue,
        PLAYER_MSGTYPE_RESP_ACK, PLAYER_SIMULATION_REQ_SET_POSE2D);
  }



  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
        PLAYER_SIMULATION_REQ_GET_POSE3D, this->device_addr))
  {
    bool response = false;
    player_simulation_pose3d_req_t *req = 
      (player_simulation_pose3d_req_t*)(data);

    this->iface->Lock(1);

    strcpy((char*)this->iface->data->model_name, req->name);
    strcpy((char*)this->iface->data->model_req,"get_pose");

    this->iface->Unlock();
   
    // Wait for response from gazebo
    while (!response)
    {
      this->iface->Lock(1);
      response = strcmp((char*)this->iface->data->model_name,"") == 0;
      this->iface->Unlock();
      usleep(100000);
    }

    this->iface->Lock(1);
    req->pose.px = this->iface->data->model_pose.x;
    req->pose.py = this->iface->data->model_pose.y;
    req->pose.pz = this->iface->data->model_pose.y;

    req->pose.proll = this->iface->data->model_pose.roll;
    req->pose.ppitch = this->iface->data->model_pose.pitch;
    req->pose.pyaw = this->iface->data->model_pose.yaw;
    this->iface->Unlock();

    this->driver->Publish(this->device_addr, respQueue,
        PLAYER_MSGTYPE_RESP_ACK, PLAYER_SIMULATION_REQ_GET_POSE3D,
        req, sizeof(*req), NULL);
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Update this interface, publish new info. This is
// called from GazeboDriver::Update
void SimulationInterface::Update()
{
  return;
}

///////////////////////////////////////////////////////////////////////////////
// Open a SHM interface when a subscription is received. This is called from
// GazeboDriver::Subscribe
void SimulationInterface::Subscribe()
{
  printf("Simulation Subscribe\n");

  // Open the interface
  try
  {
    this->iface->Open(GazeboClient::client, this->gz_id);
  }
  catch (GazeboError e)
  {
    std::ostringstream stream;
    stream <<"Error Subscribing to Gazebo Simulation Interface\n"
           << e << "\n";
    gzthrow(stream.str());
  }
}

///////////////////////////////////////////////////////////////////////////////
// Close a SHM interface. This is called from GazeboDriver::Unsubscribe
void SimulationInterface::Unsubscribe()
{
  this->iface->Close();
}
