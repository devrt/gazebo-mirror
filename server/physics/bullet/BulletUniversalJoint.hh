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
/* Desc: A universal joint
 * Author: Nate Keonig
 * Date: 24 May 2009
 * SVN: $Id:$
 */

#ifndef BULLETUNIVERSALJOINT_HH
#define BULLETUNIVERSALJOINT_HH

#include "UniversalJoint.hh"
#include "BulletJoint.hh"
#include "BulletPhysics.hh"

namespace gazebo
{
  /// \addtogroup gazebo_physics_joints
  /// \{
  /** \defgroup gazebo_universal_joint Universal Joint
   
    \brief A universal joint
  
    \par Attributes
    - body1 (string)
      - Name of the first body to attach to the joint
    - body2 (string)
      - Name of the second body to attach to the joint
    - anchor (string)
      - Name of the body which will act as the anchor to the joint
    - axis1 (float, tuple)
      - Defines the axis of rotation for the first degree of freedom
      - Default: 0 0 1
    - axis2 (float, tuple)
      - Defines the axis of rotation for the second degree of freedom
      - Default: 0 0 1
    - lowStop1 (float, degrees)
      - The low stop angle for the first degree of freedom
      - Default: infinity
    - highStop1 (float, degrees)
      - The high stop angle for the first degree of freedom
      - Default: infinity
    - lowStop2 (float, degrees)
      - The low stop angle for the second degree of freedom
      - Default: infinity
    - highStop2 (float, degrees)
      - The high stop angle for the second degree of freedom
      - Default: infinity
    - erp (double)
      - Error reduction parameter. 
      - Default = 0.4
    - cfm (double)
      - Constraint force mixing. 
      - Default = 0.8
  
  
    \par Example
    \verbatim
    <joint:universal name="universal_joint>
      <body1>body1_name</body1>
      <body2>body2_name</body2>
      <anchor>anchor_body</anchor>
      <axis1>0 0 1</axis1>
      <axis2>0 1 0</axis2>
      <lowStop1>0</lowStop1>
      <highStop1>30</highStop1>
      <lowStop2>0</lowStop2>
      <highStop2>30</highStop2>
    </joint:universal>
    \endverbatim
  */
  /// \}
  
  /// \defgroup gazebo_universal_joint Universal Joint
  /// \{
  
  /// \brief A universal joint
  class BulletUniversalJoint : public UniversalJoint<BulletJoint>
  {
    /// \brief Constructor
    public: BulletUniversalJoint(btDynamicsWorld *world);
  
    /// \brief Destuctor
    public: virtual ~BulletUniversalJoint();

    /// \brief Attach the two bodies with this joint
    public: void Attach( Body *one, Body *two );

    /// \brief Get the anchor point
    public: virtual Vector3 GetAnchor(int index) const;

    /// \brief Set the anchor point
    public: virtual void SetAnchor( int index, const Vector3 &anchor );

    /// \brief Set the first axis of rotation
    public: void SetAxis(int index, const Vector3 &axis );
  
    /// \brief Get the first axis of rotation
    public: virtual Vector3 GetAxis(int index) const;
  
    /// \brief Get the angle of axis 1
    public: virtual Angle GetAngle(int index) const;

    /// \brief Set the velocity of an axis(index).
    public: virtual void SetVelocity(int index, double angle);

    /// \brief Get the angular rate of axis 1
    public: virtual double GetVelocity(int index) const;
  
    /// \brief Set the torque of a joint.
    public: virtual void SetForce(int index, double torque);

    /// \brief Set the max allowed force of an axis(index).
    public: virtual void SetMaxForce(int index, double t);

    /// \brief Get the max allowed force of an axis(index).
    public: virtual double GetMaxForce(int index);

    /// \brief Set the high stop of an axis(index).
    public: virtual void SetHighStop(int index, Angle angle);

    /// \brief Set the low stop of an axis(index).
    public: virtual void SetLowStop(int index, Angle angle);
 
    /// \brief Get the high stop of an axis(index).
    public: virtual Angle GetHighStop(int index);

    /// \brief Get the low stop of an axis(index).
    public: virtual Angle GetLowStop(int index);
  };
  
  /// \}
}
#endif

