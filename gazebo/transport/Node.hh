/*
 * Copyright 2012 Nate Koenig
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

#ifndef _NODE_HH_
#define _NODE_HH_

#include <boost/enable_shared_from_this.hpp>
#include <map>
#include <list>
#include <string>
#include <vector>

#include "transport/TransportTypes.hh"
#include "transport/TopicManager.hh"

namespace gazebo
{
  namespace transport
  {
    /// \addtogroup gazebo_transport
    /// \{

    /// \class Node Node.hh transport/transport.hh
    /// \brief A node can advertise and subscribe topics, publish on
    ///        advertised topics and listen to subscribed topics.
    class Node : public boost::enable_shared_from_this<Node>
    {
      /// \brief Constructor
      public: Node();

      /// \brief Destructor
      public: virtual ~Node();

      /// \brief Init the node
      /// \param[in] _space Set the global namespace of all topics. If left
      ///              blank, the topic will initialize to the first
      ///              namespace on the Master
      public: void Init(const std::string &_space ="");

      /// \brief Finalize the node
      public: void Fini();

      /// \brief Get the topic namespace for this node
      /// \return The namespace
      public: std::string GetTopicNamespace() const;

      /// \brief Decode a topic name
      /// \param[in] The encoded name
      /// \return The decoded name
      public: std::string DecodeTopicName(const std::string &_topic);

      /// \brief Encode a topic name
      /// \param[in] The decoded name
      /// \return The encoded name
      public: std::string EncodeTopicName(const std::string &_topic);

      /// \brief Get the unique ID of the node
      /// \return The unique ID of the node
      public: unsigned int GetId() const;

      /// \brief Process all publishers, which has each publisher send it's
      /// most recent message over the wire. This is for internal use only
      public: void ProcessPublishers();

      /// \brief Process incoming messages.
      public: void ProcessIncoming();

      /// \brief Adverise a topic
      /// \param[in] _topic The topic to advertise
      /// \param[in] _queueLimit The maximum number of outgoing messages to
      /// queue for delivery
      /// \param[in] _latch If true, latch the last message; otherwise,
      /// don't latch
      /// \return Pointer to new publisher object
      template<typename M>
      transport::PublisherPtr Advertise(const std::string &_topic,
                                        unsigned int _queueLimit = 1000,
                                        bool _latch = false)
      {
        std::string decodedTopic = this->DecodeTopicName(_topic);
        PublisherPtr publisher =
          transport::TopicManager::Instance()->Advertise<M>(
              decodedTopic, _queueLimit, _latch);

        boost::recursive_mutex::scoped_lock lock(this->publisherMutex);
        this->publishers.push_back(publisher);
        this->publishersEnd = this->publishers.end();

        return publisher;
      }

      /// \brief Subscribe to a topic using a class method as the callback
      /// \param[in] _topic The topic to subscribe to
      /// \param[in] _fp Class method to be called on receipt of new message
      /// \param[in] _obj Class instance to be used on receipt of new message
      /// \param[in] _latching If true, latch latest incoming message;
      /// otherwise don't latch
      /// \return Pointer to new Subscriber object
      template<typename M, typename T>
      SubscriberPtr Subscribe(const std::string &_topic,
          void(T::*_fp)(const boost::shared_ptr<M const> &), T *_obj,
          bool _latching = false)
      {
        SubscribeOptions ops;
        std::string decodedTopic = this->DecodeTopicName(_topic);
        ops.template Init<M>(decodedTopic, shared_from_this(), _latching);

        boost::recursive_mutex::scoped_lock lock(this->incomingMutex);
        this->callbacks[decodedTopic].push_back(CallbackHelperPtr(
              new CallbackHelperT<M>(boost::bind(_fp, _obj, _1))));

        return transport::TopicManager::Instance()->Subscribe(ops);
      }

      /// \brief Subscribe to a topic using a bare function as the callback
      /// \param[in] _topic The topic to subscribe to
      /// \param[in] _fp Function to be called on receipt of new message
      /// \param[in] _latching If true, latch latest incoming message;
      /// otherwise don't latch
      /// \return Pointer to new Subscriber object
      template<typename M>
      SubscriberPtr Subscribe(const std::string &_topic,
          void(*_fp)(const boost::shared_ptr<M const> &),
                     bool _latching = false)
      {
        SubscribeOptions ops;
        std::string decodedTopic = this->DecodeTopicName(_topic);
        ops.template Init<M>(decodedTopic, shared_from_this(), _latching);

        boost::recursive_mutex::scoped_lock lock(this->incomingMutex);
        this->callbacks[decodedTopic].push_back(
            CallbackHelperPtr(new CallbackHelperT<M>(_fp)));

        return transport::TopicManager::Instance()->Subscribe(ops);
      }

      /// \brief Handle incoming data.
      /// \param[in] _topic Topic for which the data was received
      /// \param[in] _msg The message that was received
      /// \return true if the message was handled successfully, false otherwise
      public: bool HandleData(const std::string &_topic,
                              const std::string &_msg);

      /// \brief Add a latched message to the node for publication.
      ///
      /// This is called when a subscription is connected to a
      /// publication.
      /// \param[in] _topic Name of the topic to publish data on.
      /// \param[in] _msg The message to publish.
      public: void InsertLatchedMsg(const std::string &_topic,
                                    const std::string &_msg);


      /// \brief Get the message type for a topic
      /// \param[in] _topic The topic
      /// \return The message type
      public: std::string GetMsgType(const std::string &_topic) const;

      private: std::string topicNamespace;
      private: std::vector<PublisherPtr> publishers;
      private: std::vector<PublisherPtr>::iterator publishersIter;
      private: std::vector<PublisherPtr>::iterator publishersEnd;
      private: static unsigned int idCounter;
      private: unsigned int id;

      private: typedef std::list<CallbackHelperPtr> Callback_L;
      private: typedef std::map<std::string, Callback_L> Callback_M;
      private: Callback_M callbacks;
      private: std::map<std::string, std::list<std::string> > incomingMsgs;
      private: boost::recursive_mutex publisherMutex;
      private: boost::recursive_mutex incomingMutex;

      private: bool initialized;
    };
    /// \}
  }
}
#endif
