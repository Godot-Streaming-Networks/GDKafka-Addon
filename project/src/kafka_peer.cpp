#include "kafka_peer.hpp"

void KafkaSubscriberPeer::_bind_methods()
{
	godot::ClassDB::bind_method(godot::D_METHOD("subscribe", "topic_name"), &KafkaSubscriberPeer::Subscribe);
	godot::ClassDB::bind_method(godot::D_METHOD("receive", "data"), &KafkaSubscriberPeer::Receive);
}

const godot::Error KafkaSubscriberPeer::Subscribe(const godot::String &p_topic_name) const
{
	if (!m_subscriber) 
	{
		return godot::Error::ERR_UNCONFIGURED;
	}

	std::string topic_name = p_topic_name.utf8().get_data();
	if (!m_subscriber->Subscribe(topic_name)) 
	{
		return godot::Error::ERR_UNCONFIGURED; // Failed to subscribe to the topic.
	}

	return godot::Error::OK; // Successfully subscribed.
}

const godot::Error KafkaSubscriberPeer::Receive(godot::PackedByteArray &data) const
{
	if (!m_subscriber) 
	{
		return godot::Error::ERR_UNCONFIGURED;
	}

	std::vector<GodotStreaming::Packet> packets;
	m_subscriber->Poll(packets, 1000, 1);
	if (packets.empty()) 
	{
		return godot::Error::OK; // No packets received.
	}

	GodotStreaming::Packet packet = packets[0];
	if (packet.GetSize() == 0) 
	{
		return godot::Error::OK; // Received an empty packet.
	}

	// Resize the PackedByteArray to fit the received data.
	data.resize(packet.GetSize());

	uint8_t *write_access = data.ptrw();
	if (!write_access) 
	{
		return godot::Error::ERR_UNCONFIGURED; // Failed to get write access.
	}

	std::memcpy(write_access, packet.GetData().data(), packet.GetSize());

	return godot::Error::OK; // Successfully received data.
}

void KafkaPublisherPeer::_bind_methods()
{
	godot::ClassDB::bind_method(godot::D_METHOD("publish", "topic_name", "data", "key"), &KafkaPublisherPeer::Publish);
	godot::ClassDB::bind_method(godot::D_METHOD("purge"), &KafkaPublisherPeer::Purge);
	godot::ClassDB::bind_method(godot::D_METHOD("flush", "timeout_ms"), &KafkaPublisherPeer::Flush);
}

const godot::Error KafkaPublisherPeer::Publish(const godot::String &p_topic_name, const godot::PackedByteArray &p_data, const godot::String &p_key) const
{
    if (!m_publisher) 
    {
		return godot::Error::ERR_UNCONFIGURED;
	}

    if (p_data.is_empty()) 
	{
		return godot::Error::OK; // Nothing to send, just return OK.
	}

	std::string topic_name = p_topic_name.utf8().get_data();
	GodotStreaming::Packet packet(p_data.size(), p_data.ptr());
	std::string key = p_key.utf8().get_data();

	if (!m_publisher->Publish(topic_name, packet, key))
	{
		return godot::Error::ERR_TIMEOUT; // Failed to publish the packet.
	}

    return godot::Error::OK;
}

const godot::Error KafkaPublisherPeer::Purge()
{
	if (!m_publisher) 
	{
		return godot::Error::ERR_UNCONFIGURED;
	}

	if (!m_publisher->Purge())
	{
		return godot::Error::ERR_TIMEOUT; // Failed to purge the publisher.
	}

	return godot::Error::OK; // Successfully purged.
}

const godot::Error KafkaPublisherPeer::Flush(const uint32_t p_timeout_ms) const
{
	if (!m_publisher) 
	{
		return godot::Error::ERR_UNCONFIGURED;
	}

	if (!m_publisher->Flush(p_timeout_ms))
	{
		return godot::Error::ERR_TIMEOUT; // Failed to flush the publisher.
	}

	return godot::Error::OK; // Successfully flushed.
}

void Kafka::_bind_methods()
{
	// Severity levels for logging.
	BIND_ENUM_CONSTANT(SEVERITY_EMERGENCY);
	BIND_ENUM_CONSTANT(SEVERITY_ALERT);
	BIND_ENUM_CONSTANT(SEVERITY_CRITICAL);
	BIND_ENUM_CONSTANT(SEVERITY_ERROR);
	BIND_ENUM_CONSTANT(SEVERITY_WARNING);
	BIND_ENUM_CONSTANT(SEVERITY_NOTICE);
	BIND_ENUM_CONSTANT(SEVERITY_INFO);
	BIND_ENUM_CONSTANT(SEVERITY_DEBUG);

	// Debug Flags
	BIND_BITFIELD_FLAG(DEBUG_NONE);
	BIND_BITFIELD_FLAG(DEBUG_GENERIC);
	BIND_BITFIELD_FLAG(DEBUG_BROKER);
	BIND_BITFIELD_FLAG(DEBUG_TOPIC);
	BIND_BITFIELD_FLAG(DEBUG_METADATA);
	BIND_BITFIELD_FLAG(DEBUG_FEATURE);
	BIND_BITFIELD_FLAG(DEBUG_QUEUE);
	BIND_BITFIELD_FLAG(DEBUG_MSG);
	BIND_BITFIELD_FLAG(DEBUG_PROTOCOL);
	BIND_BITFIELD_FLAG(DEBUG_CGRP);
	BIND_BITFIELD_FLAG(DEBUG_SECURITY);
	BIND_BITFIELD_FLAG(DEBUG_FETCH);
	BIND_BITFIELD_FLAG(DEBUG_INTERCEPTOR);
	BIND_BITFIELD_FLAG(DEBUG_PLUGIN);
	BIND_BITFIELD_FLAG(DEBUG_CONSUMER);
	BIND_BITFIELD_FLAG(DEBUG_ADMIN);
	BIND_BITFIELD_FLAG(DEBUG_EOS);
	BIND_BITFIELD_FLAG(DEBUG_MOCK);
	BIND_BITFIELD_FLAG(DEBUG_ASSIGNOR);
	BIND_BITFIELD_FLAG(DEBUG_CONF);
	BIND_BITFIELD_FLAG(DEBUG_TELEMETRY);
	BIND_BITFIELD_FLAG(DEBUG_ALL);
}

void KafkaSubscriberPeerConfiguration::_bind_methods()
{
	godot::ClassDB::bind_method(godot::D_METHOD("get_brokers"), &KafkaSubscriberPeerConfiguration::get_brokers);
	godot::ClassDB::bind_method(godot::D_METHOD("set_brokers", "brokers"), &KafkaSubscriberPeerConfiguration::set_brokers);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "brokers"), "set_brokers", "get_brokers");

	godot::ClassDB::bind_method(godot::D_METHOD("get_group_id"), &KafkaSubscriberPeerConfiguration::get_group_id);
	godot::ClassDB::bind_method(godot::D_METHOD("set_group_id", "group_id"), &KafkaSubscriberPeerConfiguration::set_group_id);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "group_id"), "set_group_id", "get_group_id");

	godot::ClassDB::bind_method(godot::D_METHOD("is_group_generate_unique"), &KafkaSubscriberPeerConfiguration::is_group_generate_unique);
	godot::ClassDB::bind_method(godot::D_METHOD("set_group_generate_unique", "generate_unique"), &KafkaSubscriberPeerConfiguration::set_group_generate_unique);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "group_generate_unique"), "set_group_generate_unique", "is_group_generate_unique");

	godot::ClassDB::bind_method(godot::D_METHOD("get_topics"), &KafkaSubscriberPeerConfiguration::get_topics);
	godot::ClassDB::bind_method(godot::D_METHOD("set_topics", "topics"), &KafkaSubscriberPeerConfiguration::set_topics);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::PACKED_STRING_ARRAY, "topics"), "set_topics", "get_topics");

	godot::ClassDB::bind_method(godot::D_METHOD("get_offset"), &KafkaSubscriberPeerConfiguration::get_offset);
	godot::ClassDB::bind_method(godot::D_METHOD("set_offset", "offset"), &KafkaSubscriberPeerConfiguration::set_offset);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "offset"), "set_offset", "get_offset");

	godot::ClassDB::bind_method(godot::D_METHOD("is_auto_commit"), &KafkaSubscriberPeerConfiguration::is_auto_commit);
	godot::ClassDB::bind_method(godot::D_METHOD("set_auto_commit", "auto_commit"), &KafkaSubscriberPeerConfiguration::set_auto_commit);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_commit"), "set_auto_commit", "is_auto_commit");

	godot::ClassDB::bind_method(godot::D_METHOD("is_partition_eof_enabled"), &KafkaSubscriberPeerConfiguration::is_partition_eof_enabled);
	godot::ClassDB::bind_method(godot::D_METHOD("set_partition_eof_enabled", "partition_eof_enabled"), &KafkaSubscriberPeerConfiguration::set_partition_eof_enabled);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "partition_eof_enabled"), "set_partition_eof_enabled", "is_partition_eof_enabled");

	godot::ClassDB::bind_method(godot::D_METHOD("get_debug_flags"), &KafkaSubscriberPeerConfiguration::get_debug_flags);
	godot::ClassDB::bind_method(godot::D_METHOD("set_debug_flags", "debug_flags"), &KafkaSubscriberPeerConfiguration::set_debug_flags);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "debug_flags"), "set_debug_flags", "get_debug_flags");

	godot::ClassDB::bind_method(godot::D_METHOD("get_log_severity"), &KafkaSubscriberPeerConfiguration::get_log_severity);
	godot::ClassDB::bind_method(godot::D_METHOD("set_log_severity", "log_severity"), &KafkaSubscriberPeerConfiguration::set_log_severity);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "log_severity"), "set_log_severity", "get_log_severity");

	godot::ClassDB::bind_method(godot::D_METHOD("get_session_timeout"), &KafkaSubscriberPeerConfiguration::get_session_timeout);
	godot::ClassDB::bind_method(godot::D_METHOD("set_session_timeout", "session_timeout"), &KafkaSubscriberPeerConfiguration::set_session_timeout);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "session_timeout"), "set_session_timeout", "get_session_timeout");

	godot::ClassDB::bind_method(godot::D_METHOD("get_max_poll_interval"), &KafkaSubscriberPeerConfiguration::get_max_poll_interval);
	godot::ClassDB::bind_method(godot::D_METHOD("set_max_poll_interval", "max_poll_interval"), &KafkaSubscriberPeerConfiguration::set_max_poll_interval);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_poll_interval"), "set_max_poll_interval", "get_max_poll_interval");
}

godot::String KafkaSubscriberPeerConfiguration::get_brokers() const
{
	return godot::String();
}

void KafkaSubscriberPeerConfiguration::set_brokers(godot::String &p_brokers)
{
}

godot::String KafkaSubscriberPeerConfiguration::get_group_id() const
{
	return godot::String();
}

void KafkaSubscriberPeerConfiguration::set_group_id(godot::String &p_group_id)
{
}

bool KafkaSubscriberPeerConfiguration::is_group_generate_unique() const
{
	return false;
}

void KafkaSubscriberPeerConfiguration::set_group_generate_unique(bool p_generate_unique)
{
}

godot::PackedStringArray KafkaSubscriberPeerConfiguration::get_topics() const
{
	return godot::PackedStringArray();
}

void KafkaSubscriberPeerConfiguration::set_topics(godot::PackedStringArray &p_topics)
{
}

Kafka::Offset KafkaSubscriberPeerConfiguration::get_offset() const
{
	return Kafka::Offset();
}

void KafkaSubscriberPeerConfiguration::set_offset(Kafka::Offset p_offset)
{
}

bool KafkaSubscriberPeerConfiguration::is_auto_commit() const
{
	return false;
}

void KafkaSubscriberPeerConfiguration::set_auto_commit(bool p_auto_commit)
{
}

bool KafkaSubscriberPeerConfiguration::is_partition_eof_enabled() const
{
	return false;
}

void KafkaSubscriberPeerConfiguration::set_partition_eof_enabled(bool p_partition_eof_enabled)
{
}

Kafka::DebugFlags KafkaSubscriberPeerConfiguration::get_debug_flags() const
{
	return Kafka::DebugFlags();
}

void KafkaSubscriberPeerConfiguration::set_debug_flags(Kafka::DebugFlags p_debug_flags)
{
}

Kafka::Severity KafkaSubscriberPeerConfiguration::get_log_severity() const
{
	return Kafka::Severity();
}

void KafkaSubscriberPeerConfiguration::set_log_severity(Kafka::Severity p_log_severity)
{
}

int KafkaSubscriberPeerConfiguration::get_session_timeout() const
{
	return 0;
}

void KafkaSubscriberPeerConfiguration::set_session_timeout(int p_session_timeout)
{
}

int KafkaSubscriberPeerConfiguration::get_max_poll_interval() const
{
	return 0;
}

void KafkaSubscriberPeerConfiguration::set_max_poll_interval(int p_max_poll_interval)
{
}

void KafkaPublisherPeerConfiguration::_bind_methods()
{
}
