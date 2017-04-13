/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#include <sdf/sdf.hh>

#include "gazebo/common/Assert.hh"
#include "gazebo/rendering/ogre_gazebo.h"
#include "gazebo/rendering/Camera.hh"
#include "gazebo/rendering/DistortionPrivate.hh"
#include "gazebo/rendering/Distortion.hh"

using namespace gazebo;
using namespace rendering;

//////////////////////////////////////////////////
Distortion::Distortion()
  : dataPtr(new DistortionPrivate)
{
  this->dataPtr->k1 = 0;
  this->dataPtr->k2 = 0;
  this->dataPtr->k3 = 0;
  this->dataPtr->p1 = 0;
  this->dataPtr->p2 = 0;
  this->dataPtr->lensCenter = ignition::math::Vector2d(0.5, 0.5);
  this->dataPtr->distortionScale = ignition::math::Vector2d(1.0, 1.0);
  this->dataPtr->distortionCrop = false;
}

//////////////////////////////////////////////////
Distortion::~Distortion()
{
  delete this->dataPtr;
  this->dataPtr = NULL;
}

//////////////////////////////////////////////////
void Distortion::Load(sdf::ElementPtr _sdf)
{
  this->sdf = _sdf;
  this->dataPtr->k1 = this->sdf->Get<double>("k1");
  this->dataPtr->k2 = this->sdf->Get<double>("k2");
  this->dataPtr->k3 = this->sdf->Get<double>("k3");
  this->dataPtr->p1 = this->sdf->Get<double>("p1");
  this->dataPtr->p2 = this->sdf->Get<double>("p2");
  this->dataPtr->lensCenter =
    this->sdf->Get<ignition::math::Vector2d>("center");

  if (this->dataPtr->k1 < 0)
  {
    this->dataPtr->distortionCrop = true;
  }
  else
  {
    this->dataPtr->distortionCrop = false;
  }
}

//////////////////////////////////////////////////
ignition::math::Vector2d
    Distortion::DistortionMapValueClamped(const int x, const int y) const
{
  if (x < 0 || x >= static_cast<int>(this->dataPtr->distortionTexWidth) ||
      y < 0 || y >= static_cast<int>(this->dataPtr->distortionTexHeight))
  {
    return ignition::math::Vector2d(-1, -1);
  }
  ignition::math::Vector2d res =
      this->dataPtr->distortionMap[y*this->dataPtr->distortionTexWidth+x];
  return res;
}

//////////////////////////////////////////////////
void Distortion::SetCamera(CameraPtr _camera)
{
  if (!_camera)
  {
    gzerr << "Unable to apply distortion, camera is NULL" << std::endl;
    return;
  }

  // seems to work best with a square distortion map texture
  unsigned int texSide = _camera->ImageHeight() > _camera->ImageWidth() ?
      _camera->ImageHeight() : _camera->ImageWidth();
  this->dataPtr->distortionTexWidth = texSide - 1;
  this->dataPtr->distortionTexHeight = texSide - 1;
  unsigned int imageSize =
      this->dataPtr->distortionTexWidth * this->dataPtr->distortionTexHeight;
  double incrU = 1.0 / this->dataPtr->distortionTexWidth;
  double incrV = 1.0 / this->dataPtr->distortionTexHeight;

  // initialize distortion map
  this->dataPtr->distortionMap.resize(imageSize);
  for (unsigned int i = 0; i < this->dataPtr->distortionMap.size(); ++i)
  {
    this->dataPtr->distortionMap[i] = -1;
  }

  // fill the distortion map
  for (unsigned int i = 0; i < this->dataPtr->distortionTexHeight; ++i)
  {
    double v = i*incrV;
    for (unsigned int j = 0; j < this->dataPtr->distortionTexWidth; ++j)
    {
      double u = j*incrU;
      ignition::math::Vector2d uv(u, v);
      ignition::math::Vector2d out = this->Distort(
          uv,
          this->dataPtr->lensCenter,
          this->dataPtr->k1, this->dataPtr->k2, this->dataPtr->k3,
          this->dataPtr->p1, this->dataPtr->p2);

      // compute the index in the distortion map
      unsigned int idxU = out.X() * this->dataPtr->distortionTexWidth;
      unsigned int idxV = out.Y() * this->dataPtr->distortionTexHeight;

      if (idxU < this->dataPtr->distortionTexWidth &&
          idxV < this->dataPtr->distortionTexHeight)
      {
        unsigned int mapIdx = idxV * this->dataPtr->distortionTexWidth + idxU;
        this->dataPtr->distortionMap[mapIdx] = uv;
      }
      // else: pixel maps outside the image bounds.
      // This is expected and normal to ensure
      // no black borders; carry on
    }
  }

  // set up the distortion instance
  Ogre::MaterialPtr distMat =
      Ogre::MaterialManager::getSingleton().getByName(
      "Gazebo/CameraDistortionMap");
  distMat =
      distMat->clone("Gazebo/" + _camera->Name() + "_CameraDistortionMap");

  // create the distortion map texture for the distortion instance
  std::string texName = _camera->Name() + "_distortionTex";
  Ogre::TexturePtr renderTexture =
      Ogre::TextureManager::getSingleton().createManual(
          texName,
          "General",
          Ogre::TEX_TYPE_2D,
          this->dataPtr->distortionTexWidth,
          this->dataPtr->distortionTexHeight,
          0,
          Ogre::PF_FLOAT32_RGB);
  Ogre::HardwarePixelBufferSharedPtr pixelBuffer = renderTexture->getBuffer();

  // fill the distortion map, while interpolating to fill dead pixels
  pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL);
  const Ogre::PixelBox &pixelBox = pixelBuffer->getCurrentLock();
  float *pDest = static_cast<float *>(pixelBox.data);
  for (unsigned int i = 0; i < this->dataPtr->distortionTexHeight; ++i)
  {
    for (unsigned int j = 0; j < this->dataPtr->distortionTexWidth; ++j)
    {
      ignition::math::Vector2d vec =
          this->dataPtr->distortionMap[i*this->dataPtr->distortionTexWidth+j];

      // perform interpolation on-the-fly:
      // check for empty mapping within the region and correct it by
      // interpolating the eight neighboring distortion map values.

      if (vec.X() < -0.5 && vec.Y() < -0.5)
      {
        ignition::math::Vector2d left =
            this->DistortionMapValueClamped(j-1, i);
        ignition::math::Vector2d right =
            this->DistortionMapValueClamped(j+1, i);
        ignition::math::Vector2d bottom =
            this->DistortionMapValueClamped(j, i+1);
        ignition::math::Vector2d top =
            this->DistortionMapValueClamped(j, i-1);

        ignition::math::Vector2d topLeft =
            this->DistortionMapValueClamped(j-1, i-1);
        ignition::math::Vector2d topRight =
            this->DistortionMapValueClamped(j+1, i-1);
        ignition::math::Vector2d bottomLeft =
            this->DistortionMapValueClamped(j-1, i+1);
        ignition::math::Vector2d bottomRight =
            this->DistortionMapValueClamped(j+1, i+1);


        ignition::math::Vector2d interpolated;
        double divisor = 0;
        if (right.X() > -0.5)
        {
          divisor++;
          interpolated += right;
        }
        if (left.X() > -0.5)
        {
          divisor++;
          interpolated += left;
        }
        if (top.X() > -0.5)
        {
          divisor++;
          interpolated += top;
        }
        if (bottom.X() > -0.5)
        {
          divisor++;
          interpolated += bottom;
        }

        if (bottomRight.X() > -0.5)
        {
          divisor += 0.707;
          interpolated += bottomRight * 0.707;
        }
        if (bottomLeft.X() > -0.5)
        {
          divisor += 0.707;
          interpolated += bottomLeft * 0.707;
        }
        if (topRight.X() > -0.5)
        {
          divisor += 0.707;
          interpolated += topRight * 0.707;
        }
        if (topLeft.X() > -0.5)
        {
          divisor += 0.707;
          interpolated += topLeft * 0.707;
        }

        if (divisor > 0.5)
        {
          interpolated /= divisor;
        }
        *pDest++ = ignition::math::clamp(interpolated.X(), 0.0, 1.0);
        *pDest++ = ignition::math::clamp(interpolated.Y(), 0.0, 1.0);
      }
      else
      {
        *pDest++ = vec.X();
        *pDest++ = vec.Y();
      }

      // Z coordinate
      *pDest++ = 0;
    }
  }
  pixelBuffer->unlock();

  if (this->dataPtr->distortionCrop)
  {
    // I believe that if not used with a square distortion texture, this
    // calculation will result in stretching of the final output image.
    ignition::math::Vector2d boundA = this->Distort(
        ignition::math::Vector2d(0, 0),
        this->dataPtr->lensCenter,
        this->dataPtr->k1, this->dataPtr->k2, this->dataPtr->k3,
        this->dataPtr->p1, this->dataPtr->p2);
    ignition::math::Vector2d boundB = this->Distort(
        ignition::math::Vector2d(1, 1),
        this->dataPtr->lensCenter,
        this->dataPtr->k1, this->dataPtr->k2, this->dataPtr->k3,
        this->dataPtr->p1, this->dataPtr->p2);
    this->dataPtr->distortionScale = boundB - boundA;

    // Both invalid: scale very close to 0 OR negative scale
    if (this->dataPtr->distortionScale.X() < 1e-7 ||
        this->dataPtr->distortionScale.Y() < 1e-7)
    {
      gzerr << "Distortion model attempted to apply a scale parameter of ("
            << this->dataPtr->distortionScale.X() << ", "
            << this->dataPtr->distortionScale.Y() << ", which is invalid.\n";
    }
    else
    {
      Ogre::GpuProgramParametersSharedPtr params =
          distMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
      params->setNamedConstant("scale",
          Ogre::Vector3(1.0/this->dataPtr->distortionScale.X(),
          1.0/this->dataPtr->distortionScale.Y(), 1.0));
    }
  }

  // set up the distortion map texture to be used in the pixel shader.
  distMat->getTechnique(0)->getPass(0)->createTextureUnitState(texName, 1);

  // these lines should come after the distortion map is applied to distMat.
  this->dataPtr->lensDistortionInstance =
      Ogre::CompositorManager::getSingleton().addCompositor(
      _camera->OgreViewport(), "CameraDistortionMap/Default");
  this->dataPtr->lensDistortionInstance->getTechnique()->getOutputTargetPass()->
      getPass(0)->setMaterial(distMat);
  this->dataPtr->lensDistortionInstance->setEnabled(true);
}

//////////////////////////////////////////////////
ignition::math::Vector2d Distortion::Distort(
    const ignition::math::Vector2d &_in,
    const ignition::math::Vector2d &_center, double _k1, double _k2, double _k3,
    double _p1, double _p2)
{
  // apply Brown's distortion model, see
  // http://en.wikipedia.org/wiki/Distortion_%28optics%29#Software_correction

  ignition::math::Vector2d normalized2d = _in - _center;
  ignition::math::Vector3d normalized(normalized2d.X(), normalized2d.Y(), 0);
  double rSq = normalized.X() * normalized.X()
      + normalized.Y() * normalized.Y();

  // radial
  ignition::math::Vector3d dist = normalized * (1.0 +
      _k1 * rSq +
      _k2 * rSq * rSq +
      _k3 * rSq * rSq * rSq);

  // tangential
  dist.X() += _p2 * (rSq + 2 * (normalized.X()*normalized.X())) +
      2 * _p1 * normalized.X() * normalized.Y();
  dist.Y() += _p1 * (rSq + 2 * (normalized.Y()*normalized.Y())) +
      2 * _p2 * normalized.X() * normalized.Y();
  ignition::math::Vector2d out =
      _center + ignition::math::Vector2d(dist.X(), dist.Y());

  return out;
}

//////////////////////////////////////////////////
void Distortion::SetCrop(bool _crop)
{
  this->dataPtr->distortionCrop = _crop;
}

//////////////////////////////////////////////////
double Distortion::GetK1() const
{
  return this->dataPtr->k1;
}

//////////////////////////////////////////////////
double Distortion::GetK2() const
{
  return this->dataPtr->k2;
}

//////////////////////////////////////////////////
double Distortion::GetK3() const
{
  return this->dataPtr->k3;
}

//////////////////////////////////////////////////
double Distortion::GetP1() const
{
  return this->dataPtr->p1;
}

//////////////////////////////////////////////////
double Distortion::GetP2() const
{
  return this->dataPtr->p2;
}

//////////////////////////////////////////////////
bool Distortion::Crop() const
{
  return this->dataPtr->distortionCrop;
}

//////////////////////////////////////////////////
ignition::math::Vector2d Distortion::GetCenter() const
{
  return this->dataPtr->lensCenter;
}
