#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <optional>
#include <functional>
#include <thread>
#include <stdexcept>
#include <unordered_map>
#include <memory>

// Includes rdkafka.h from the rdkafka external library.
enum rd_kafka_OffsetSpec_t;
// Includes rdkafkacpp.h from the rdkafka external library.
namespace RdKafka {
	class KafkaConsumer;
	class Producer;
	class Message;
	class Topic;
	class TopicPartition;
	class Event;
	enum Severity 
	{
		EVENT_SEVERITY_EMERG = 0,
		EVENT_SEVERITY_ALERT = 1,
		EVENT_SEVERITY_CRITICAL = 2,
		EVENT_SEVERITY_ERROR = 3,
		EVENT_SEVERITY_WARNING = 4,
		EVENT_SEVERITY_NOTICE = 5,
		EVENT_SEVERITY_INFO = 6,
		EVENT_SEVERITY_DEBUG = 7
	};
	enum ErrorCode;
	enum OffsetSpec_t 
	{
		OFFSET_BEGINNING = -2,
		OFFSET_END = -1,
		OFFSET_STORED = -1000
	};
}

namespace GodotStreaming 
{
	class Packet; // Forward declaration for the Packet class.
	template<typename T>
	class Result; // Forward declaration for the Result class.
	class Status; // Forward declaration for the Status class.

	namespace Internal 
	{
		class Logger;
		class RebalanceCb;
		class DeliveryReportCb;
	};

	struct KafkaSubscriberMetadata 
	{
	public:
		std::string brokers; // Comma-separated list of Kafka brokers to connect to, e.g., "broker1:9092,broker2:9092".
		std::vector<std::string> topics; // List of topics to subscribe to. If empty, the consumer will not subscribe to any topics initially.

		std::optional<std::string> group_id; // Optional group ID for the consumer, if not provided, a unique group ID will be generated.
		bool group_generate_unique = true; // If true, a unique group ID will be generated if none is provided. This is useful for testing or when you want to avoid conflicts with existing consumer groups.

		RdKafka::OffsetSpec_t offset_reset; // Specifies the offset reset behavior when no initial offset is found or the current offset is out of range.
		bool enable_auto_commit = true; // If true, the consumer will automatically commit offsets after processing messages.

		bool enable_partition_eof = false; // If true, the consumer will receive EOF events when it reaches the end of a partition.

		std::string debug; // Debugging options for the consumer, can include "all", "topic", "msg", etc. This is useful for debugging and logging purposes.

		uint64_t session_timeout_ms = 10000; // Session timeout in milliseconds. This is the maximum time the consumer can be inactive before it is considered dead.
		uint64_t max_poll_interval_ms = 300000; // Maximum time between poll calls before the consumer is considered dead. This is useful for detecting long-running operations that may block the consumer.

		RdKafka::Severity severity_log_level; // The severity level for logging messages. This can be used to filter log messages based on their importance.
		std::function<void(const RdKafka::Message &)> delivery_report_callback; // Callback function to handle delivery reports for messages sent by the producer. This is useful for tracking message delivery status.
		std::function<void(const RdKafka::Severity logLevel, const std::string &message)> logger_callback; // Callback function for logging messages. This allows the consumer to log messages with a specific severity level and message content.
		std::function<void(RdKafka::KafkaConsumer *, RdKafka::ErrorCode, std::vector<RdKafka::TopicPartition *> &)> rebalance_callback; // Callback function for handling rebalances. This is called when the consumer's partition assignment changes, allowing the application to handle partition reassignment.
	};

	struct KafkaPublisherMetadata 
	{
	public:
		std::string brokers;
		RdKafka::Severity severity_log_level;
		std::function<void(const RdKafka::Message &)> delivery_report_callback;
		std::function<void(const RdKafka::Severity logLevel, const std::string &message)> logger_callback;

		bool flush_immediately = false; // If true, the producer will flush messages immediately after sending them. This can be useful for ensuring that messages are sent before the application exits or before a critical operation completes.
		uint32_t flush_rate = 100; // If flush_immedidately is false, this is the amount of messages before the producer will flush messages. This is useful for batching messages to improve performance and reduce network overhead.
		uint64_t flush_timeout_ms = 1000; // Timeout in milliseconds for flushing messages. If the producer does not flush within this time, it will return an error.
	};

	class KafkaPublisher 
	{
	public:
		KafkaPublisher(RdKafka::Producer *p_producer, const KafkaPublisherMetadata &p_metadata)
			: m_producer(p_producer), m_metadata(p_metadata)
		{
			if (!m_producer) 
			{
				throw std::runtime_error("Failed to create KafkaPublisher: Producer or Topic is null.");
			}
		}
		~KafkaPublisher()
		{
			Close();
		}

		static Result<KafkaPublisher> Create(const KafkaPublisherMetadata &p_metadata);

		Status Publish(const std::string &topic, const Packet &p_packet, const std::string &p_key = "");
		Status Purge(); // Purges the producer's message queue, removing all messages without sending them.
		Status Flush(uint32_t p_timeout_ms = 1000); // Flushes the producer's message queue, ensuring all messages are sent before proceeding.
		
		void Close();
	private:
		KafkaPublisherMetadata m_metadata;
		RdKafka::Producer *m_producer = nullptr; // Pointer to the Kafka producer instance.
		std::unordered_map<std::string, RdKafka::Topic *> m_topics = std::unordered_map<std::string, RdKafka::Topic*>(); // Pointer to the Kafka topic instance.

		Internal::Logger *m_logger = nullptr; // Pointer to the logger instance for logging messages.
		Internal::DeliveryReportCb *m_delivery_report_cb = nullptr; // Pointer to the delivery report callback instance.

		uint32_t m_message_count = 0; // Counter for the number of messages sent by this publisher.
	};

	class KafkaSubscriber 
	{
	public:
		KafkaSubscriber(RdKafka::KafkaConsumer *p_consumer, const KafkaSubscriberMetadata &p_metadata);
		~KafkaSubscriber();

		static Result<KafkaSubscriber> Create(const KafkaSubscriberMetadata &p_metadata);

		Status Subscribe(const std::string &p_topic);
		Status Subscribe_Array(const std::vector<std::string> &p_topics);

		Status Poll(std::vector<Packet> &p_packets, uint32_t p_timeout_ms = 1000, uint32_t p_max_packets = UINT32_MAX);
		void Close();
	private:
		KafkaSubscriberMetadata m_metadata;
		RdKafka::KafkaConsumer *m_consumer = nullptr; // Pointer to the Kafka m_consumer instance.

		Internal::Logger *m_logger = nullptr; // Pointer to the logger instance for logging messages.
		Internal::RebalanceCb *m_rebalance_cb = nullptr; // Pointer to the rebalance callback instance.
		Internal::DeliveryReportCb *m_delivery_report_cb = nullptr; // Pointer to the delivery report callback instance.
	};

	class KafkaController 
	{
	public:
		KafkaController() = default;
		~KafkaController()
		{
			Close();
		}

		Result<KafkaPublisher> CreatePublisher(const KafkaPublisherMetadata &p_metadata);
		Result<KafkaSubscriber> CreateConsumer(const KafkaSubscriberMetadata &p_metadata);

		void Close();
	private:

		// Callback for error handling.
		std::function<void(const RdKafka::Severity severity, const std::string &)> m_error_callback;
		RdKafka::Severity m_error_log_level;

		// This class can manage multiple m_publishers and m_subscribers if needed.
		std::vector<std::shared_ptr<KafkaPublisher>> m_publishers;
		std::vector<std::shared_ptr<KafkaSubscriber>> m_subscribers;
	};

	template<typename T>
	struct PacketSerializer
	{
		/// <summary>
		/// Serialization function for a value of type T.
		/// </summary>
		/// <param name="p_packet">Refenence Packet that is being serialized to...</param>
		/// <param name="p_value">By Reference</param>
		static void Serialize(Packet &p_packet, const T &p_value) = delete;

		/// <summary>
		/// Deserialization function for a value of type T.
		/// 
		/// Moves the pointer forward in the packet data to the next available position after deserialization.
		/// </summary>
		/// <param name="p_packet">Reference Packet that is being deserialized from...</param>
		/// <returns>A value of type T that was deserialized from the packet.</returns>
		static Result<T> Deserialize(Packet &p_packet) = delete;
	};

	// Forward declarations for specializations to override =delete behavior
	template<> struct PacketSerializer<int8_t>;
	template<> struct PacketSerializer<uint8_t>;
	template<> struct PacketSerializer<int16_t>;
	template<> struct PacketSerializer<uint16_t>;
	template<> struct PacketSerializer<int32_t>;
	template<> struct PacketSerializer<uint32_t>;
	template<> struct PacketSerializer<int64_t>;
	template<> struct PacketSerializer<uint64_t>;
	template<> struct PacketSerializer<float>;
	template<> struct PacketSerializer<double>;
	template<> struct PacketSerializer<bool>;
	template<> struct PacketSerializer<std::string>;

	extern template struct PacketSerializer<int8_t>;
	extern template struct PacketSerializer<uint8_t>;
	extern template struct PacketSerializer<int16_t>;
	extern template struct PacketSerializer<uint16_t>;
	extern template struct PacketSerializer<int32_t>;
	extern template struct PacketSerializer<uint32_t>;
	extern template struct PacketSerializer<int64_t>;
	extern template struct PacketSerializer<uint64_t>;
	extern template struct PacketSerializer<float>;
	extern template struct PacketSerializer<std::string>;

	class Packet 
	{
	public:
		Packet() = default;
		Packet(size_t p_size, const void *p_data)
		{
			m_data.resize(p_size);
			if (p_data) 
			{
				std::memcpy(m_data.data(), p_data, p_size);
			}

			m_pointer = 0;
		}
		~Packet() = default;

		void Reset() 
		{
			m_pointer = 0;
		}

		template<typename T>
		inline void Serialize(const T &p_data)
		{
			PacketSerializer<T>::Serialize(*this, p_data);
		}

		template<typename T>
		Result<T> Deserialize()
		{
			return PacketSerializer<T>::Deserialize(*this);
		}

		const size_t GetSize() const
		{
			return m_data.size();
		}
		std::span<const std::byte> GetData() const
		{
			return std::span<const std::byte>(m_data.data(), m_data.size());
		}
	private:
		std::vector<std::byte> m_data;
		std::size_t m_pointer = 0;

		// Grants access to all PacketSerializer<T> specializations to the private members of Packet.
		template<typename T>
		friend struct PacketSerializer;
	};

	template<typename T>
	class Result 
	{
	public:
		bool success;
		std::shared_ptr<T> value;
		std::string error_message;

		static Result<T> Ok(std::shared_ptr<T> value)
		{
			return Result<T>{ true, std::move(value), "" };
		}
		static Result<T> Error(const std::string &error_message)
		{
			return Result<T>{ false, nullptr, error_message };
		}

		operator bool() const 
		{
			return success;
		}
	};

	class Status 
	{ 
	public:
		bool success;
		std::string message;

		static Status Ok() 
		{
			return Status{ true, "" };
		}
		static Status Error(const std::string &message) 
		{
			return Status{ false, message };
		}

		operator bool() const 
		{
			return success;
		}
	};

	template<>
	struct PacketSerializer<int8_t>
	{
		static void Serialize(Packet &p_packet, const int8_t &p_data)
		{
			p_packet.m_data.resize(p_packet.m_data.size() + sizeof(int8_t)); // Ensure enough space in the packet data for int8_t
			std::memcpy(p_packet.m_data.data() + p_packet.m_data.size() - sizeof(int8_t), &p_data, sizeof(int8_t)); // Copy the int8_t data to the end of the packet data
		}

		static Result<int8_t> Deserialize(Packet &p_packet)
		{
			// Check if the packet data is large enough to hold an int8_t
			if (p_packet.m_data.size() < sizeof(int8_t) + p_packet.m_pointer)
			{
				return Result<int8_t>::Error("Packet size is too small for int8_t deserialization.");
			}

			int8_t result = 0;
			// Copy the data from the pointer position in the packet data
			std::memcpy(&result, p_packet.m_data.data() + p_packet.m_pointer, sizeof(int8_t));
			// Move the pointer forward by the size of int8_t
			p_packet.m_pointer += sizeof(int8_t);

			return Result<int8_t>::Ok(std::make_unique<int8_t>(result));
		}
	};

	template<>
	struct PacketSerializer<uint8_t>
	{
		static void Serialize(Packet &p_packet, const uint8_t &p_data)
		{
			p_packet.m_data.resize(p_packet.m_data.size() + sizeof(uint8_t)); // Ensure enough space in the packet data for uint8_t
			std::memcpy(p_packet.m_data.data() + p_packet.m_data.size() - sizeof(uint8_t), &p_data, sizeof(uint8_t)); // Copy the uint8_t data to the end of the packet data
		}

		static Result<uint8_t> Deserialize(Packet &p_packet)
		{
			// Check if the packet data is large enough to hold a uint8_t
			if (p_packet.m_data.size() < sizeof(uint8_t) + p_packet.m_pointer)
			{
				return Result<uint8_t>::Error("Packet size is too small for uint8_t deserialization.");
			}

			uint8_t result = 0;
			// Copy the data from the pointer position in the packet data
			std::memcpy(&result, p_packet.m_data.data() + p_packet.m_pointer, sizeof(uint8_t));
			// Move the pointer forward by the size of uint8_t
			p_packet.m_pointer += sizeof(uint8_t);

			return Result<uint8_t>::Ok(std::make_unique<uint8_t>(result));
		}
	};

	template<>
	struct PacketSerializer<int16_t>
	{
		static void Serialize(Packet &p_packet, const int16_t &p_data)
		{
			p_packet.m_data.resize(p_packet.m_data.size() + sizeof(int16_t)); // Ensure enough space in the packet data for int16_t
			std::memcpy(p_packet.m_data.data() + p_packet.m_data.size() - sizeof(int16_t), &p_data, sizeof(int16_t)); // Copy the int16_t data to the end of the packet data
		}

		static Result<int16_t> Deserialize(Packet &p_packet)
		{
			// Check if the packet data is large enough to hold an int16_t
			if (p_packet.m_data.size() < sizeof(int16_t) + p_packet.m_pointer)
			{
				return Result<int16_t>::Error("Packet size is too small for int16_t deserialization.");
			}

			int16_t result = 0;
			// Copy the data from the pointer position in the packet data
			std::memcpy(&result, p_packet.m_data.data() + p_packet.m_pointer, sizeof(int16_t));
			// Move the pointer forward by the size of int16_t
			p_packet.m_pointer += sizeof(int16_t);

			return Result<int16_t>::Ok(std::make_unique<int16_t>(result));
		}
	};

	template<>
	struct PacketSerializer<uint16_t>
	{
		static void Serialize(Packet &p_packet, const uint16_t &p_data)
		{
			p_packet.m_data.resize(p_packet.m_data.size() + sizeof(uint16_t)); // Ensure enough space in the packet data for uint16_t
			std::memcpy(p_packet.m_data.data() + p_packet.m_data.size() - sizeof(uint16_t), &p_data, sizeof(uint16_t)); // Copy the uint16_t data to the end of the packet data
		}

		static Result<uint16_t> Deserialize(Packet &p_packet)
		{
			// Check if the packet data is large enough to hold a uint16_t
			if (p_packet.m_data.size() < sizeof(uint16_t) + p_packet.m_pointer)
			{
				return Result<uint16_t>::Error("Packet size is too small for uint16_t deserialization.");
			}

			uint16_t result = 0;
			// Copy the data from the pointer position in the packet data
			std::memcpy(&result, p_packet.m_data.data() + p_packet.m_pointer, sizeof(uint16_t));
			// Move the pointer forward by the size of uint16_t
			p_packet.m_pointer += sizeof(uint16_t);

			return Result<uint16_t>::Ok(std::make_unique<uint16_t>(result));
		}
	};

	template<>
	struct PacketSerializer<int32_t>
	{
		static void Serialize(Packet &p_packet, const int32_t &p_data)
		{
			p_packet.m_data.resize(p_packet.m_data.size() + sizeof(int32_t)); // Ensure enough space in the packet data for int32_t
			std::memcpy(p_packet.m_data.data() + p_packet.m_data.size() - sizeof(int32_t), &p_data, sizeof(int32_t)); // Copy the int32_t data to the end of the packet data
		}

		static Result<int32_t> Deserialize(Packet &p_packet)
		{
			// Check if the packet data is large enough to hold an int32_t
			if (p_packet.m_data.size() < sizeof(int32_t) + p_packet.m_pointer)
			{
				return Result<int32_t>::Error("Packet size is too small for int32_t deserialization.");
			}

			int32_t result = 0;
			// Copy the data from the pointer position in the packet data
			std::memcpy(&result, p_packet.m_data.data() + p_packet.m_pointer, sizeof(int32_t));
			// Move the pointer forward by the size of int32_t
			p_packet.m_pointer += sizeof(int32_t);

			return Result<int32_t>::Ok(std::make_unique<int32_t>(result));
		}
	};

	template<>
	struct PacketSerializer<uint32_t>
	{
		static void Serialize(Packet &p_packet, const uint32_t &p_data)
		{
			p_packet.m_data.resize(p_packet.m_data.size() + sizeof(uint32_t)); // Ensure enough space in the packet data for uint32_t
			std::memcpy(p_packet.m_data.data() + p_packet.m_data.size() - sizeof(uint32_t), &p_data, sizeof(uint32_t)); // Copy the uint32_t data to the end of the packet data
		}

		static Result<uint32_t> Deserialize(Packet &p_packet)
		{
			// Check if the packet data is large enough to hold a uint32_t
			if (p_packet.m_data.size() < sizeof(uint32_t) + p_packet.m_pointer)
			{
				return Result<uint32_t>::Error("Packet size is too small for uint32_t deserialization.");
			}

			uint32_t result = 0;
			// Copy the data from the pointer position in the packet data
			std::memcpy(&result, p_packet.m_data.data() + p_packet.m_pointer, sizeof(uint32_t));
			// Move the pointer forward by the size of uint32_t
			p_packet.m_pointer += sizeof(uint32_t);

			return Result<uint32_t>::Ok(std::make_unique<uint32_t>(result));
		}
	};


	template<>
	struct PacketSerializer<int64_t>
	{
		static void Serialize(Packet &p_packet, const int64_t &p_data)
		{
			p_packet.m_data.resize(p_packet.m_data.size() + sizeof(int64_t)); // Ensure enough space in the packet data for int64_t
			std::memcpy(p_packet.m_data.data() + p_packet.m_data.size() - sizeof(int64_t), &p_data, sizeof(int64_t)); // Copy the int64_t data to the end of the packet data
		}

		static Result<int64_t> Deserialize(Packet &p_packet)
		{
			// Check if the packet data is large enough to hold an int64_t
			if (p_packet.m_data.size() < sizeof(int64_t) + p_packet.m_pointer)
			{
				return Result<int64_t>::Error("Packet size is too small for int64_t deserialization.");
			}

			int64_t result = 0;
			// Copy the data from the pointer position in the packet data
			std::memcpy(&result, p_packet.m_data.data() + p_packet.m_pointer, sizeof(int64_t));
			// Move the pointer forward by the size of int64_t
			p_packet.m_pointer += sizeof(int64_t);

			return Result<int64_t>::Ok(std::make_unique<int64_t>(result));
		}
	};

	template<>
	struct PacketSerializer<uint64_t>
	{
		static void Serialize(Packet &p_packet, const uint64_t &p_data)
		{
			p_packet.m_data.resize(p_packet.m_data.size() + sizeof(uint64_t)); // Ensure enough space in the packet data for uint64_t
			std::memcpy(p_packet.m_data.data() + p_packet.m_data.size() - sizeof(uint64_t), &p_data, sizeof(uint64_t)); // Copy the uint64_t data to the end of the packet data
		}

		static Result<uint64_t> Deserialize(Packet &p_packet)
		{
			// Check if the packet data is large enough to hold a uint64_t
			if (p_packet.m_data.size() < sizeof(uint64_t) + p_packet.m_pointer)
			{
				return Result<uint64_t>::Error("Packet size is too small for uint64_t deserialization.");
			}

			uint64_t result = 0;
			// Copy the data from the pointer position in the packet data
			std::memcpy(&result, p_packet.m_data.data() + p_packet.m_pointer, sizeof(uint64_t));
			// Move the pointer forward by the size of uint64_t
			p_packet.m_pointer += sizeof(uint64_t);

			return Result<uint64_t>::Ok(std::make_unique<uint64_t>(result));
		}
	};
}