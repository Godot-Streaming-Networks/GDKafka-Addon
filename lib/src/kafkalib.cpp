#include "kafkalib.hpp"

#include <rdkafkacpp.h>

#include <stdexcept>

namespace GodotStreaming 
{


#pragma region Internal

	class Internal::DeliveryReportCb : public RdKafka::DeliveryReportCb
	{
	public:
		~DeliveryReportCb() = default;

		void set_callback(std::function<void(const RdKafka::Message &)> cb)
		{
			m_callback = cb;
		}

		void dr_cb(RdKafka::Message &message) override
		{
			if (m_callback)
			{
				m_callback(message);
			}
		}

	private:
		std::function<void(const RdKafka::Message &)> m_callback;
	};

	class Internal::RebalanceCb : public RdKafka::RebalanceCb
	{
	public:
		RebalanceCb() = default;
		virtual ~RebalanceCb() override = default;

		// Copy and move constructor
		RebalanceCb(const RebalanceCb &other)  // Copy constructor
		{
			m_callback = other.m_callback;
		}
		RebalanceCb(RebalanceCb &&other) noexcept // Move constructor
		{
			m_callback = std::move(other.m_callback);
		}

		void set_callback(const std::function<void(RdKafka::KafkaConsumer *, RdKafka::ErrorCode, std::vector<RdKafka::TopicPartition *> &)> cb)
		{
			m_callback = cb;
		}

		void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
			std::vector<RdKafka::TopicPartition *> &partitions) override
		{
			if (err == RdKafka::ERR__ASSIGN_PARTITIONS)
			{
				consumer->assign(partitions);
			}
			else
			{
				consumer->unassign();
			}

			if (m_callback)
			{
				m_callback(consumer, err, partitions);
			}
		}
	private:
		std::function<void(RdKafka::KafkaConsumer *, RdKafka::ErrorCode, std::vector<RdKafka::TopicPartition *> &)> m_callback;
	};

	class Internal::Logger : public RdKafka::EventCb
	{
	public:
		~Logger() = default;

		void set_callback(const std::function<void(const RdKafka::Severity logLevel, const std::string &message)> cb)
		{
			m_callback = cb;
		}

		void event_cb(RdKafka::Event &event) override
		{
			if (m_callback)
			{
				m_callback(static_cast<RdKafka::Severity>(event.severity()), event.str());
			}
		}
	private:
		std::function<void(const RdKafka::Severity logLevel, const std::string &message)> m_callback;
	};

#pragma endregion

#pragma region KafkaPublisher
	Result<KafkaPublisher> KafkaPublisher::Create(const KafkaPublisherMetadata &p_metadata)
	{
		if (p_metadata.brokers.empty()) 
		{
			return Result<KafkaPublisher>::Error("Brokers cannot be empty.");
		}

		RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
		RdKafka::Conf::ConfResult err;
		std::string errstr;

		// Bootstrap servers
		err = conf->set("bootstrap.servers", p_metadata.brokers, errstr);
		if (err != RdKafka::Conf::CONF_OK)
		{
			return Result<KafkaPublisher>::Error("Failed to set bootstrap servers: " + errstr);
		}
		// Logging
		err = conf->set("log_level", std::to_string(static_cast<int>(p_metadata.severity_log_level)), errstr);
		if (err != RdKafka::Conf::CONF_OK)
		{
			return Result<KafkaPublisher>::Error("Failed to set log level: " + errstr);
		}
		// Callbacks
		Internal::DeliveryReportCb *m_delivery_report_cb = nullptr;
		if (p_metadata.delivery_report_callback) 
		{
			m_delivery_report_cb = new Internal::DeliveryReportCb();
			m_delivery_report_cb->set_callback(p_metadata.delivery_report_callback);
			err = conf->set("dr_cb", m_delivery_report_cb, errstr);
			if (err != RdKafka::Conf::CONF_OK) 
			{
				delete m_delivery_report_cb; // Clean up delivery report callback if setting fails
				delete conf; // Clean up configuration if setting the callback fails
				return Result<KafkaPublisher>::Error("Failed to set delivery report callback: " + errstr);
			}
		}
		Internal::Logger *m_logger = nullptr;
		if (p_metadata.logger_callback) 
		{
			m_logger = new Internal::Logger();
			m_logger->set_callback(p_metadata.logger_callback);
			err = conf->set("event_cb", m_logger, errstr);
			if (err != RdKafka::Conf::CONF_OK) 
			{
				if (m_delivery_report_cb) 
				{
					delete m_delivery_report_cb; // Clean up delivery report callback if logger creation fails
				}

				delete conf; // Clean up configuration if setting the logger fails

				return Result<KafkaPublisher>::Error("Failed to set logger callback: " + errstr);
			}
		}

		// Create the producer
		RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
		if (!producer) 
		{
			if (m_delivery_report_cb) 
			{
				delete m_delivery_report_cb; // Clean up delivery report callback if producer creation fails
			}
			if (m_logger) 
			{
				delete m_logger; // Clean up logger if it was created
			}
			delete conf; // Clean up configuration if producer creation fails
			return Result<KafkaPublisher>::Error("Failed to create producer: " + errstr);
		}

		delete conf; // Clean up configuration after producer creation

		std::shared_ptr<KafkaPublisher> kafkaPublisher = std::make_shared<KafkaPublisher>(producer, p_metadata);

		kafkaPublisher->m_logger = m_logger; // Store the logger if it was created
		kafkaPublisher->m_delivery_report_cb = m_delivery_report_cb; // Store the delivery report callback if it was created

		return Result<KafkaPublisher>::Ok(std::move(kafkaPublisher));
	}
	Status KafkaPublisher::Publish(const std::string &topic, const Packet &p_packet, const std::string &p_key)
	{
		RdKafka::Topic *m_topic = nullptr;
		if (m_topics.find(topic) != m_topics.end()) 
		{
			// Use existing topic if it exists
			m_topic = m_topics[topic];
		} 
		else 
		{
			// Create a new topic if it does not exist
			std::string errstr;

			m_topic = RdKafka::Topic::create(m_producer, topic, nullptr, errstr);
			if (!m_topic) 
			{
				return Status::Error("Failed to create and publish to topic: " + errstr);
			}

			m_topics[topic] = m_topic; // Store the topic in the map
		}

		std::span<const std::byte> bytes = p_packet.GetData();

		if (bytes.empty()) 
		{
			return Status::Error("Packet data is empty. Cannot publish an empty packet.");
		}

		std::string *key_ptr = nullptr; // Pointer to hold the key if provided
		if (!p_key.empty()) 
		{
			key_ptr = const_cast<std::string*>(&p_key); // Use the key if provided
		}

		RdKafka::ErrorCode err = m_producer->produce(
			m_topic,
			RdKafka::Topic::PARTITION_UA, // Use unassigned partition
			RdKafka::Producer::RK_MSG_COPY, // Copy the message
			const_cast<std::byte *>(bytes.data()), // Data to send
			bytes.size(), // Size of data
			key_ptr, // Key
			nullptr // Opaque (not used)
		);
		if (err != RdKafka::ERR_NO_ERROR) 
		{
			return Status::Error("Failed to produce message: " + RdKafka::err2str(err));
		}

		// Flush
		if (m_metadata.flush_immediately || m_message_count >= m_metadata.flush_rate)
		{
			m_message_count = 0; // Reset the message count after flushing
			err = m_producer->flush(m_metadata.flush_timeout_ms);
			if (err != RdKafka::ERR_NO_ERROR) 
			{
				return Status::Error("Failed to flush producer: " + RdKafka::err2str(err));
			}
		}
		m_message_count++; // Increment the message count

		return Status::Ok(); // Return success status
	}
	Status KafkaPublisher::Purge() 
	{
		RdKafka::ErrorCode err = m_producer->purge(RdKafka::Producer::PURGE_INFLIGHT); // Purge any in-flight messages
		if (err != RdKafka::ERR_NO_ERROR) 
		{
			return Status::Error("Failed to purge producer: " + RdKafka::err2str(err));
		}
		
		return Status::Ok(); // Return success status after purging
	}
	Status KafkaPublisher::Flush(uint32_t p_timeout_ms) 
	{
		RdKafka::ErrorCode err = m_producer->flush(p_timeout_ms); // Flush the producer with the specified timeout
		if (err != RdKafka::ERR_NO_ERROR) 
		{
			return Status::Error("Failed to flush producer: " + RdKafka::err2str(err));
		}

		return Status::Ok(); // Return success status after flushing
	}
	void KafkaPublisher::Close()
	{
		for (auto &pair : m_topics) 
		{
			if (pair.second) 
			{
				delete pair.second; // Clean up each topic
			}
		}
		m_topics.clear();

		if (m_producer)
		{
			m_producer->purge(RdKafka::Producer::PURGE_INFLIGHT); // Purge any in-flight messages
			m_producer->flush(1000); // Wait for up to 1000 ms for messages to be sent
			delete m_producer; // Clean up producer
			m_producer = nullptr;
		}

		if (m_delivery_report_cb)
		{
			delete m_delivery_report_cb; // Clean up delivery report callback if it was created
			m_delivery_report_cb = nullptr;
		}
		if (m_logger)
		{
			delete m_logger; // Clean up logger if it was created
			m_logger = nullptr;
		}
	}
#pragma endregion

#pragma region KafkaConsumer
	KafkaSubscriber::KafkaSubscriber(RdKafka::KafkaConsumer *p_consumer, const KafkaSubscriberMetadata &p_metadata)
		: m_consumer(p_consumer), m_metadata(p_metadata)
	{
		if (!m_consumer) 
		{
			throw std::runtime_error("KafkaConsumer cannot be created with a null consumer.");
		}
	
	}
	KafkaSubscriber::~KafkaSubscriber()
	{
		Close();
	}
	Result<KafkaSubscriber> KafkaSubscriber::Create(const KafkaSubscriberMetadata &p_metadata)
	{
		if (p_metadata.brokers.empty()) 
		{
			return Result<KafkaSubscriber>::Error("Brokers cannot be empty.");
		}

		auto now = std::chrono::system_clock::now();
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
		RdKafka::Conf::ConfResult err = RdKafka::Conf::CONF_OK;
		std::string errstr;

		// Debug
		// Enable debug logging
		if (!p_metadata.debug.empty()) 
		{
			conf->set("debug", p_metadata.debug, errstr); // or use "consumer,cgrp,topic" for more focused logs
			if (err != RdKafka::Conf::CONF_OK) 
			{
				delete conf; // Clean up configuration if setting debug fails
				return Result<KafkaSubscriber>::Error("Failed to set debug: " + errstr);
			}
		}

		// Timeouts
		err = conf->set("session.timeout.ms", std::to_string(p_metadata.session_timeout_ms), errstr);
		if (err != RdKafka::Conf::CONF_OK) 
		{
			delete conf; // Clean up configuration if setting session timeout fails
			return Result<KafkaSubscriber>::Error("Failed to set session timeout: " + errstr);
		}
		err = conf->set("max.poll.interval.ms", std::to_string(p_metadata.max_poll_interval_ms), errstr);
		if (err != RdKafka::Conf::CONF_OK) 
		{
			delete conf; // Clean up configuration if setting max poll interval fails
			return Result<KafkaSubscriber>::Error("Failed to set max poll interval: " + errstr);
		}

		// Bootstrap servers
		err = conf->set("bootstrap.servers", p_metadata.brokers, errstr);
		if (err != RdKafka::Conf::CONF_OK) 
		{
			delete conf; // Clean up configuration if setting bootstrap servers fails
			return Result<KafkaSubscriber>::Error("Failed to set bootstrap servers: " + errstr);
		}
		
		// Logging
		err = conf->set("log_level", std::to_string(static_cast<int>(p_metadata.severity_log_level)), errstr);
		if (err != RdKafka::Conf::CONF_OK) 
		{
			delete conf; // Clean up configuration if setting log level fails
			return Result<KafkaSubscriber>::Error("Failed to set log level: " + errstr);
		}

		// Auto offset reset
		std::string auto_offset_reset = p_metadata.offset_reset == RdKafka::OffsetSpec_t::OFFSET_BEGINNING ? "earliest" : "latest";
		err = conf->set("auto.offset.reset", auto_offset_reset, errstr);
		if (err != RdKafka::Conf::CONF_OK)
		{
			delete conf; // Clean up configuration if setting auto offset reset fails
			// Return error
			return Result<KafkaSubscriber>::Error("Failed to set auto offset reset: " + errstr);
		}
		// Enable auto commit
		err = conf->set("enable.auto.commit", p_metadata.enable_auto_commit ? "true" : "false", errstr);
		if (err != RdKafka::Conf::CONF_OK)
		{
			delete conf; // Clean up configuration if setting auto commit fails
			return Result<KafkaSubscriber>::Error("Failed to set enable auto commit: " + errstr);
		}

		// Group ID
		std::string group_id = p_metadata.group_id.value_or("default_group");
		if (p_metadata.group_generate_unique)
		{
			// Generate a random group ID if requested
			group_id += "_" + std::to_string(now_ms);
		}
		err = conf->set("group.id", group_id, errstr);
		if (err != RdKafka::Conf::CONF_OK)
		{
			delete conf; // Clean up configuration if setting group ID fails
			return Result<KafkaSubscriber>::Error("Failed to set group ID: " + errstr);
		}

		// Callbacks
		Internal::DeliveryReportCb *m_delivery_report_cb = nullptr;
		if (p_metadata.delivery_report_callback) 
		{
			// Create and set the delivery report callback
			m_delivery_report_cb = new Internal::DeliveryReportCb();
			m_delivery_report_cb->set_callback(p_metadata.delivery_report_callback);
			err = conf->set("dr_cb", m_delivery_report_cb, errstr);
			if (err != RdKafka::Conf::CONF_OK) 
			{
				delete m_delivery_report_cb; // Clean up delivery report callback if setting fails
				delete conf; // Clean up configuration if setting the callback fails
				return Result<KafkaSubscriber>::Error("Failed to set delivery report callback: " + errstr);
			}
		}
		Internal::Logger *m_logger = nullptr;
		if (p_metadata.logger_callback) 
		{
			m_logger = new Internal::Logger();
			m_logger->set_callback(p_metadata.logger_callback);
			err = conf->set("event_cb", m_logger, errstr);
			if (err != RdKafka::Conf::CONF_OK) 
			{
				if (m_delivery_report_cb) 
				{
					delete m_delivery_report_cb; // Clean up delivery report callback if logger creation fails
				}
				delete m_logger; // Clean up logger if it was created
				delete conf; // Clean up configuration if setting the logger fails
				return Result<KafkaSubscriber>::Error("Failed to set logger callback: " + errstr);
			}
		}
		Internal::RebalanceCb *m_rebalance_cb = nullptr;
		if (p_metadata.rebalance_callback) 
		{
			m_rebalance_cb = new Internal::RebalanceCb();
			m_rebalance_cb->set_callback(p_metadata.rebalance_callback);
			err = conf->set("rebalance_cb", m_rebalance_cb, errstr);
			if (err != RdKafka::Conf::CONF_OK) 
			{
				if (m_delivery_report_cb) 
				{
					delete m_delivery_report_cb; // Clean up delivery report callback if rebalance callback creation fails
				}
				if (m_logger) 
				{
					delete m_logger; // Clean up logger if it was created
				}
				delete m_rebalance_cb; // Clean up rebalance callback if it was created
				delete conf; // Clean up configuration if setting the rebalance callback fails
				return Result<KafkaSubscriber>::Error("Failed to set rebalance callback: " + errstr);
			}
		}

		// Create the consumer
		RdKafka::KafkaConsumer *consumer = RdKafka::KafkaConsumer::create(conf, errstr);
		if (!consumer) 
		{
			if (m_delivery_report_cb) 
			{
				delete m_delivery_report_cb; // Clean up delivery report callback if consumer creation fails
			}
			if (m_logger) 
			{
				delete m_logger; // Clean up logger if it was created
			}
			if (m_rebalance_cb) 
			{
				delete m_rebalance_cb; // Clean up rebalance callback if it was created
			}
			delete conf; // Clean up configuration if consumer creation fails
			return Result<KafkaSubscriber>::Error("Failed to create consumer: " + errstr);
		}

		delete conf; // Clean up configuration after consumer creation

		RdKafka::ErrorCode resp = consumer->subscribe(p_metadata.topics);
		if (resp != RdKafka::ERR_NO_ERROR) 
		{
			if (m_delivery_report_cb) 
			{
				delete m_delivery_report_cb; // Clean up delivery report callback if subscription fails
			}
			if (m_logger) 
			{
				delete m_logger; // Clean up logger if it was created
			}
			if (m_rebalance_cb) 
			{
				delete m_rebalance_cb; // Clean up rebalance callback if it was created
			}
			delete consumer; // Clean up consumer if subscription fails
			return Result<KafkaSubscriber>::Error("Failed to subscribe to topics: " + RdKafka::err2str(resp));
		}

		std::shared_ptr<KafkaSubscriber> kafkaConsumer = std::make_shared<KafkaSubscriber>(consumer, p_metadata);

		kafkaConsumer->m_logger = m_logger; // Store the logger if it was created
		kafkaConsumer->m_rebalance_cb = m_rebalance_cb; // Store the rebalance callback if it was created
		kafkaConsumer->m_delivery_report_cb = m_delivery_report_cb; // Store the delivery report callback if it was created

		return Result<KafkaSubscriber>::Ok(std::move(kafkaConsumer));
	}
	Status KafkaSubscriber::Subscribe(const std::string &p_topic)
	{
		if (!m_consumer) 
		{
			return Status::Error("Consumer is not initialized.");
		}

		RdKafka::ErrorCode resp = m_consumer->subscribe({p_topic});
		if (resp != RdKafka::ERR_NO_ERROR) 
		{
			return Status::Error("Failed to subscribe to topic '" + p_topic + "': " + RdKafka::err2str(resp));
		}

		return Status::Ok(); // Return success status after subscribing
	}
	Status KafkaSubscriber::Subscribe_Array(const std::vector<std::string> &p_topics)
	{
		if (!m_consumer) 
		{
			return Status::Error("Consumer is not initialized.");
		}

		RdKafka::ErrorCode resp = m_consumer->subscribe(p_topics);
		if (resp != RdKafka::ERR_NO_ERROR) 
		{
			return Status::Error("Failed to subscribe to topics: " + RdKafka::err2str(resp));
		}

		return Status::Ok(); // Return success status after subscribing
	}
	Status KafkaSubscriber::Poll(std::vector<Packet> &p_packets, uint32_t p_timeout_ms, uint32_t p_max_packets)
	{
		if (!m_consumer) 
		{
			return Status::Error("Consumer is not initialized.");
		}

		// Poll for messages with the specified timeout
		RdKafka::Message *message = nullptr;
		uint32_t count = 0;
		do 
		{

			message = m_consumer->consume(p_timeout_ms);
			if (!message) 
			{
				break;
			}

			//if (message->err() != RdKafka::ERR_NO_ERROR) 
			//{
			//	RdKafka::ErrorCode err = message->err();
			//	delete message; // Clean up message
			//	std::string topics;
			//	for (const auto &topic : m_metadata.topics) 
			//	{
			//		topics += topic + ", ";
			//	}
			//	return Status::Error("Error consuming message: " + RdKafka::err2str(err) + " ("+ std::to_string(p_timeout_ms) + "ms timeout) [" + m_metadata.brokers + " | "+ topics + " | " + m_metadata.group_id.value_or("<NULL/DEFAULT>") + "]");
			//}

			std::span<const std::byte> bytes(reinterpret_cast<const std::byte *>(message->payload()), message->len());
			if (bytes.empty()) 
			{
				delete message; // Clean up message
				break;
			}

			// Create a new packet with the received data
			Packet packet(bytes.size(), bytes.data());

			p_packets.emplace_back(packet); // Create a new packet with the received data
			
			delete message; // Clean up message
			message = nullptr;
			count++;
		} while (count < p_max_packets);

		return Status::Ok(); // Return success status after processing all messages
	}
	void KafkaSubscriber::Close()
	{
		if (m_consumer) 
		{
			m_consumer->close();
			m_consumer = nullptr;
		}

		if (m_rebalance_cb) 
		{
			delete m_rebalance_cb; // Clean up rebalance callback if it was created
			m_rebalance_cb = nullptr;
		}
		if (m_delivery_report_cb) 
		{
			delete m_delivery_report_cb; // Clean up delivery report callback if it was created
			m_delivery_report_cb = nullptr;
		}
		if (m_logger) 
		{
			delete m_logger; // Clean up logger if it was created
			m_logger = nullptr;
		}
	}
#pragma endregion

#pragma region KafkaController
	// --- Implement Kafka Controller --- 
	Result<KafkaPublisher> KafkaController::CreatePublisher(const KafkaPublisherMetadata &p_metadata)
	{
		Result<KafkaPublisher> publisher = KafkaPublisher::Create(p_metadata);
		if (!publisher) 
		{
			return publisher; // Return the error if publisher creation failed
		}
		m_publishers.push_back(publisher.value);
		return publisher;
	}
	Result<KafkaSubscriber> KafkaController::CreateConsumer(const KafkaSubscriberMetadata &p_metadata)
	{
		Result<KafkaSubscriber> subscriber = KafkaSubscriber::Create(p_metadata);
		if (!subscriber) 
		{
			return subscriber; // Return the error if subscriber creation failed
		}
		m_subscribers.push_back(subscriber.value);
		return subscriber;
	}
	void KafkaController::Close()
	{
		for (auto &publisher : m_publishers) {
			publisher->Close();
		}
		m_publishers.clear();

		for (auto &subscriber : m_subscribers) {
			subscriber->Close();
		}
		m_subscribers.clear();
	}
#pragma endregion
}