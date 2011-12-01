#include "transport/TopicManager.hh"
#include "transport/ConnectionManager.hh"
#include "transport/PublicationTransport.hh"

using namespace gazebo;
using namespace transport;

int PublicationTransport::counter = 0;

PublicationTransport::PublicationTransport(const std::string &topic, 
                                           const std::string &msgType)
 : topic(topic), msgType(msgType)
{
  this->id = counter++;
  TopicManager::Instance()->UpdatePublications(topic, msgType);
}

PublicationTransport::~PublicationTransport()
{
  if (this->connection)
  {
    this->connection->DisconnectShutdown(this->shutdownConnectionPtr);

    msgs::Subscribe sub;
    sub.set_topic(this->topic);
    sub.set_msg_type(this->msgType);
    sub.set_host( this->connection->GetLocalAddress() );
    sub.set_port( this->connection->GetLocalPort() );
    ConnectionManager::Instance()->Unsubscribe( sub );
    this->connection->Cancel();
    this->connection.reset();

    ConnectionManager::Instance()->RemoveConnection( this->connection );
  }
  this->callback.clear();
}

void PublicationTransport::Init(const ConnectionPtr &conn_)
{
  this->connection = conn_;
  msgs::Subscribe sub;
  sub.set_topic(this->topic);
  sub.set_msg_type(this->msgType);
  sub.set_host( this->connection->GetLocalAddress() );
  sub.set_port( this->connection->GetLocalPort() );

  this->connection->EnqueueMsg( msgs::Package("sub",sub) );

  // Put this in PublicationTransportPtr
  // Start reading messages from the remote publisher
  this->connection->AsyncRead(boost::bind(&PublicationTransport::OnPublish, this, _1));

  this->shutdownConnectionPtr = this->connection->ConnectToShutdown(
      boost::bind(&PublicationTransport::OnConnectionShutdown, this));
}

void PublicationTransport::OnConnectionShutdown()
{
  gzdbg << "Publication transport connection shutdown\n";
}

void PublicationTransport::AddCallback(const boost::function<void(const std::string &)> &cb_)
{
  this->callback = cb_;
}

void PublicationTransport::OnPublish(const std::string &data)
{
  std::cout << "PublicationTransport::OnPublish[" << this->topic << "]\n";
  if (this->connection && this->connection->IsOpen())
  {
    this->connection->AsyncRead( 
        boost::bind(&PublicationTransport::OnPublish, this, _1) );

    if (!data.empty())
    {
      if (this->callback)
        (this->callback)(data);
    }
  }
}

const ConnectionPtr PublicationTransport::GetConnection() const
{
  return this->connection;
}

std::string PublicationTransport::GetTopic() const
{
  return this->topic;
}

std::string PublicationTransport::GetMsgType() const
{
  return this->msgType;
}

void PublicationTransport::Fini()
{
  /// Cancel all async operatiopns.
  this->connection->Cancel();
  this->connection.reset();
}


