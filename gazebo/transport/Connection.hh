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
#ifndef _CONNECTION_HH_
#define _CONNECTION_HH_

#include <google/protobuf/message.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <deque>


#include "common/Event.hh"
#include "common/Console.hh"
#include "common/Exception.hh"

#define HEADER_LENGTH 8

namespace gazebo
{
  namespace transport
  {
    extern bool is_stopped();

    class IOManager;
    class Connection;
    typedef boost::shared_ptr<Connection> ConnectionPtr;

    /// \addtogroup gazebo_transport Transport
    /// \{

    /// \class Connection Connection.hh transport/transport.hh
    /// \brief Single TCP/IP connection manager
    class Connection : public boost::enable_shared_from_this<Connection>
    {
      /// \brief Constructor
      public: Connection();

      /// \brief Destructor
      public: virtual ~Connection();

      /// \brief Connect to a remote host
      /// \param[in] _host The host to connect to
      /// \param[in] _port The port to connect to
      /// \return true if connection succeeded, false otherwise
      public: bool Connect(const std::string &_host, unsigned int _port);

      /// \brief The signature of a connection accept callback
      typedef boost::function<void(const ConnectionPtr&)> AcceptCallback;

      /// \brief Start a server that listens on a port
      /// \param[in] _port The port to listen on
      /// \param[in] _accept_cb The callback to invoke when a new connection has
      /// been accepted
      public: void Listen(unsigned int _port, const AcceptCallback &_accept_cb);

      /// \brief The signature of a connection read callback
      typedef boost::function<void(const std::string &_data)> ReadCallback;

      /// \brief Start a thread that reads from the connection and passes
      ///        new message to the ReadCallback
      /// \param[in] _cb The callback to invoke when a new message is received
      public: void StartRead(const ReadCallback &_cb);

      /// \brief Stop the read loop
      public: void StopRead();

      /// \brief Shutdown the socket
      public: void Shutdown();

      /// \brief Is the connection open?
      /// \return true if the connection is open; false otherwise
      public: bool IsOpen() const;

      /// \brief Close a connection
      private: void Close();

      /// \brief Cancel all async operations on an open socket
      public: void Cancel();

      /// \brief Read data from the socket
      /// \param[out] _data Destination for data that is read
      /// \return true if data was successfully read, false otherwise
      public: bool Read(std::string &_data);

      /// \brief Write data to the socket
      /// \param[in] _buffer Data to write
      /// \param[in] _force If true, block until the data has been written
      /// to the socket, otherwise just enqueue the data for asynchronous write
      public: void EnqueueMsg(const std::string &_buffer, bool _force = false);

      /// \brief Get the local URI
      /// \return The local URI
      public: std::string GetLocalURI() const;

      /// \brief Get the remote URI
      /// \return The remote URI
      public: std::string GetRemoteURI() const;

      /// \brief Get the local address of this connection
      /// \return The local address
      public: std::string GetLocalAddress() const;

      /// \brief Get the port of this connection
      /// \return The local port
      public: unsigned int GetLocalPort() const;

      /// \brief Get the remote address
      /// \return The remote address
      public: std::string GetRemoteAddress() const;

      /// \brief Get the remote port number
      /// \return The remote port
      public: unsigned int GetRemotePort() const;

      /// \brief Get the remote hostname
      /// \return The remote hostname
      public: std::string GetRemoteHostname() const;

      /// \brief Get the local hostname
      /// \return The local hostname
      public: std::string GetLocalHostname() const;

      /// \brief Peform an asyncronous read
      /// param[in] _handler Callback to invoke on received data
      public: template<typename Handler>
              void AsyncRead(Handler _handler)
              {
                if (!this->IsOpen())
                {
                  gzerr << "AsyncRead on a closed socket\n";
                  return;
                }

                void (Connection::*f)(const boost::system::error_code &,
                    boost::tuple<Handler>) = &Connection::OnReadHeader<Handler>;

                this->inbound_header.resize(HEADER_LENGTH);
                boost::asio::async_read(*this->socket,
                    boost::asio::buffer(this->inbound_header),
                    boost::bind(f, this,
                                boost::asio::placeholders::error,
                                boost::make_tuple(_handler)));
              }

      /// \brief Handle a completed read of a message header.
      ///
      /// The handler is passed using a tuple since boost::bind seems to
      /// have trouble binding a function object created using boost::bind
      /// as a parameter
      /// \param[in] _e Error code, if any, associated with the read
      /// \param[in] _handler Callback to invoke on received data
      private: template<typename Handler>
               void OnReadHeader(const boost::system::error_code &_e,
                                 boost::tuple<Handler> _handler)
              {
                if (_e)
                {
                  if (_e.message() != "End of File")
                  {
                    this->Close();
                    // This will occur when the other side closes the
                    // connection
                  }
                }
                else
                {
                  std::size_t inbound_data_size = 0;
                  std::string header(&this->inbound_header[0],
                                      this->inbound_header.size());
                  this->inbound_header.clear();

                  inbound_data_size = this->ParseHeader(header);

                 if (inbound_data_size > 0)
                  {
                    // Start the asynchronous call to receive data
                    this->inbound_data.resize(inbound_data_size);

                    void (Connection::*f)(const boost::system::error_code &e,
                        boost::tuple<Handler>) =
                      &Connection::OnReadData<Handler>;

                    boost::asio::async_read(*this->socket,
                        boost::asio::buffer(this->inbound_data),
                        boost::bind(f, this,
                                    boost::asio::placeholders::error,
                                    _handler));
                  }
                  else
                  {
                    gzerr << "Header is empty\n";
                    boost::get<0>(_handler)("");
                    // This code tries to read the header again. We should
                    // never get here.
                    // this->inbound_header.resize(HEADER_LENGTH);

                    // void (Connection::*f)(const boost::system::error_code &,
                    // boost::tuple<Handler>) =
                    // &Connection::OnReadHeader<Handler>;

                    // boost::asio::async_read(*this->socket,
                    //    boost::asio::buffer(this->inbound_header),
                    //    boost::bind(f, this,
                    //      boost::asio::placeholders::error, _handler));
                  }
                }
              }

      /// \brief Handle a completed read of a message body.
      ///
      /// The handler is passed using a tuple since boost::bind seems to
      /// have trouble binding a function object created using boost::bind
      /// as a parameter
      /// \param[in] _e Error code, if any, associated with the read
      /// \param[in] _handler Callback to invoke on received data
      private: template<typename Handler>
               void OnReadData(const boost::system::error_code &_e,
                              boost::tuple<Handler> _handler)
              {
                if (_e)
                  gzerr << "Error Reading data!\n";

                // Inform caller that data has been received
                std::string data(&this->inbound_data[0],
                                  this->inbound_data.size());
                this->inbound_data.clear();

                if (data.empty())
                  gzerr << "OnReadData got empty data!!!\n";

                if (!_e && !transport::is_stopped())
                {
                  boost::get<0>(_handler)(data);
                }
              }

     /// \brief Register a function to be called when the connection is shut
     /// down \param[in] _subscriber Function to be called \return Handle
     /// that can be used to unregister the function
     public: event::ConnectionPtr ConnectToShutdown(boost::function<void()>
                 _subscriber)
             { return this->shutdown.Connect(_subscriber); }

     /// \brief Unregister a function to be called when the connection is
     /// shut down \param[in] _subscriber Handle previously returned by
     /// ConnectToShutdown()
     public: void DisconnectShutdown(event::ConnectionPtr _subscriber)
             {this->shutdown.Disconnect(_subscriber);}

     /// \brief Handle on-write callbacks
     public: void ProcessWriteQueue();

     private: void OnWrite(const boost::system::error_code &e,
                  boost::asio::streambuf *_b);

     /// \brief Handle new connections, if this is a server
     /// \param[in] _e Error code for accept method
     /// \TODO Nate check
     private: void OnAccept(const boost::system::error_code &_e);

     /// \brief Parse a header to get the size of a packet
     /// \param[in] _header Header as a string
     private: std::size_t ParseHeader(const std::string &_header);

     /// \brief the read thread
     private: void ReadLoop(const ReadCallback &_cb);

     /// \brief Get the local endpoint
     /// \return The endpoint
     private: boost::asio::ip::tcp::endpoint GetLocalEndpoint() const;

     /// \brief Get the remote endpoint
     /// \return The endpoint
     private: boost::asio::ip::tcp::endpoint GetRemoteEndpoint() const;

     /// \brief Gets hostname
     /// \param[in] _ep The end point to get the hostename of
     private: static std::string GetHostname(boost::asio::ip::tcp::endpoint _ep);

     /// \brief Callback method when connected
     /// \param[in] _error Error code thrown during connection
     /// \param[in] _endPointIter Pointer to resolver iterator
     /// \TODO Nate check
     private: void OnConnect(const boost::system::error_code &_error,
                  boost::asio::ip::tcp::resolver::iterator _endPointIter);

      private: boost::asio::ip::tcp::socket *socket;
      private: boost::asio::ip::tcp::acceptor *acceptor;

      private: std::deque<std::string> writeQueue;
      private: std::deque<unsigned int> writeCounts;
      private: boost::mutex *connectMutex;
      private: boost::recursive_mutex *writeMutex;
      private: boost::recursive_mutex *readMutex;

      private: boost::condition_variable connectCondition;

      // Called when a new connection is received
      private: AcceptCallback acceptCB;

      private: std::vector<char> inbound_header;
      private: std::vector<char> inbound_data;

      private: boost::thread *readThread;
      private: bool readQuit;

      public: unsigned int id;
      private: static unsigned int idCounter;
      private: ConnectionPtr acceptConn;

      private: event::EventT<void()> shutdown;
      private: static IOManager *iomanager;

      public: unsigned int writeCount;

      private: std::string localURI;
      private: std::string localAddress;
      private: std::string remoteURI;
      private: std::string remoteAddress;

      private: bool connectError;
    };
    /// \}
  }
}

#endif


