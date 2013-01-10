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

#ifndef _DOOR_ITEM_HH_
#define _DOOR_ITEM_HH_

#include "gazebo/gui/qt.h"
#include "gazebo/gui/model_editor/RectItem.hh"
#include "gazebo/gui/model_editor/BuildingItem.hh"

namespace gazebo
{
  namespace gui
  {
    class RectItem;

    class BuildingItem;

    class WindowDoorInspectorDialog;

    class DoorItem : public RectItem, public BuildingItem
    {
        Q_OBJECT

        public: DoorItem();

        public: ~DoorItem();

        public: virtual QVector3D GetSize() const;

        public: virtual QVector3D GetScenePosition() const;

        public: virtual double GetSceneRotation() const;

        private: virtual void paint(QPainter *_painter,
            const QStyleOptionGraphicsItem *_option, QWidget *_widget);

        private: void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *_event);

        private slots: void OnApply();

        private slots: void OnOpenInspector();

        private slots: void OnDeleteItem();

        private: void DoorChanged();

        private: void SizeChanged();

        private: double doorDepth;

        private: double doorHeight;

        private: double doorWidth;

        private: double doorElevation;

        private: QPointF doorPos;

        private: double scale;

        private: WindowDoorInspectorDialog *inspector;
    };
  }
}

#endif
