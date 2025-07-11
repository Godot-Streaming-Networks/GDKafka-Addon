#pragma once

#include <kafkalib.hpp>

#include <godot_cpp/classes/packet_peer.hpp>
#include <godot_cpp/classes/resource.hpp>

class Kafka : public godot::RefCounted
{
	GDCLASS(Kafka, godot::RefCounted);
protected:
	static void _bind_methods();
public:
	enum Severity {
		SEVERITY_EMERGENCY = RdKafka::Severity::EVENT_SEVERITY_EMERG,
		SEVERITY_ALERT = RdKafka::Severity::EVENT_SEVERITY_ALERT,
		SEVERITY_CRITICAL = RdKafka::Severity::EVENT_SEVERITY_CRITICAL,
		SEVERITY_ERROR = RdKafka::Severity::EVENT_SEVERITY_ERROR,
		SEVERITY_WARNING = RdKafka::Severity::EVENT_SEVERITY_WARNING,
		SEVERITY_NOTICE = RdKafka::Severity::EVENT_SEVERITY_NOTICE,
		SEVERITY_INFO = RdKafka::Severity::EVENT_SEVERITY_INFO,
		SEVERITY_DEBUG = RdKafka::Severity::EVENT_SEVERITY_DEBUG
	};
	enum DebugFlags {
		DEBUG_NONE = (1 << 0), // 0
		DEBUG_GENERIC = (1 << 1), // 1
		DEBUG_BROKER = (1 << 2), // 2
		DEBUG_TOPIC = (1 << 3), // 4
		DEBUG_METADATA = (1 << 4), // 8
		DEBUG_FEATURE = (1 << 5), // 16
		DEBUG_QUEUE = (1 << 6), // 32
		DEBUG_MSG = (1 << 7), // 64
		DEBUG_PROTOCOL = (1 << 8), // 128
		DEBUG_CGRP = (1 << 9), // 256
		DEBUG_SECURITY = (1 << 10), // 512
		DEBUG_FETCH = (1 << 11), // 1024
		DEBUG_INTERCEPTOR = (1 << 12), // 2048
		DEBUG_PLUGIN = (1 << 13), // 4096
		DEBUG_CONSUMER = (1 << 14), // 8192
		DEBUG_ADMIN = (1 << 15), // 16384
		DEBUG_EOS = (1 << 16), // 32768
		DEBUG_MOCK = (1 << 17), // 65536
		DEBUG_ASSIGNOR = (1 << 18), // 131072
		DEBUG_CONF = (1 << 19), // 262144
		DEBUG_TELEMETRY = (1 << 20), // 524288
		// All debug flags combined
		DEBUG_ALL = (1 << 21) - 1 // 1048575
	};
	enum Offset
	{
		OFFSET_BEGINNING = RdKafka::OffsetSpec_t::OFFSET_BEGINNING,
		OFFSET_END = RdKafka::OffsetSpec_t::OFFSET_END
	};
};

VARIANT_ENUM_CAST(Kafka::Severity);
VARIANT_BITFIELD_CAST(Kafka::DebugFlags);
VARIANT_ENUM_CAST(Kafka::Offset);

class KafkaSubscriberPeerConfiguration final : public godot::Resource
{
	GDCLASS(KafkaSubscriberPeerConfiguration, godot::Resource);
protected:
	static void _bind_methods();
public:
	KafkaSubscriberPeerConfiguration() {}
	~KafkaSubscriberPeerConfiguration() {}

	godot::String get_brokers() const;
	void set_brokers(godot::String &p_brokers);
	
	godot::String get_group_id() const;
	void set_group_id(godot::String &p_group_id);
	
	bool is_group_generate_unique() const;
	void set_group_generate_unique(bool p_generate_unique);

	godot::PackedStringArray get_topics() const;
	void set_topics(godot::PackedStringArray &p_topics);

	Kafka::Offset get_offset() const;
	void set_offset(Kafka::Offset p_offset);

	bool is_auto_commit() const;
	void set_auto_commit(bool p_auto_commit);

	bool is_partition_eof_enabled() const;
	void set_partition_eof_enabled(bool p_partition_eof_enabled);

	Kafka::DebugFlags get_debug_flags() const;
	void set_debug_flags(Kafka::DebugFlags p_debug_flags);

	Kafka::Severity get_log_severity() const;
	void set_log_severity(Kafka::Severity p_log_severity);

	int get_session_timeout() const;
	void set_session_timeout(int p_session_timeout);

	int get_max_poll_interval() const;
	void set_max_poll_interval(int p_max_poll_interval);
private:
	GodotStreaming::KafkaSubscriberMetadata m_subscriber_metadata;
};

class KafkaPublisherPeerConfiguration final : public godot::Resource
{
	GDCLASS(KafkaPublisherPeerConfiguration, godot::Resource);
protected:
	static void _bind_methods();
public:
	KafkaPublisherPeerConfiguration() {}
	~KafkaPublisherPeerConfiguration() {}
	const godot::String get_brokers() const;
private:
	GodotStreaming::KafkaPublisherMetadata m_publisher_metadata;
};

class KafkaSubscriberPeer final : public godot::PacketPeer
{
	GDCLASS(KafkaSubscriberPeer, godot::PacketPeer);
protected:
	static void _bind_methods();
public:
	static KafkaSubscriberPeer *create(const godot::Ref<KafkaSubscriberPeerConfiguration> &p_configuration);

	KafkaSubscriberPeer() {}
	~KafkaSubscriberPeer() {}

	const godot::Error Subscribe(const godot::String &p_topic_name) const;

	const godot::Error Receive(godot::PackedByteArray &data) const;
private:
	std::unique_ptr<GodotStreaming::KafkaSubscriber> m_subscriber = nullptr;
};

class KafkaPublisherPeer final : public godot::PacketPeer
{
	GDCLASS(KafkaPublisherPeer, godot::PacketPeer);
protected:
	static void _bind_methods();
public:
	static KafkaPublisherPeer *create(const godot::Ref<KafkaPublisherPeerConfiguration> &p_configuration);

	KafkaPublisherPeer() {}
	~KafkaPublisherPeer() {}

	const godot::Error Publish(const godot::String &p_topic_name, const godot::PackedByteArray &p_data, const godot::String &p_key = "") const;
	const godot::Error Purge();
	const godot::Error Flush(const uint32_t p_timeout_ms) const;
private:
	std::unique_ptr<GodotStreaming::KafkaPublisher> m_publisher = nullptr;
};