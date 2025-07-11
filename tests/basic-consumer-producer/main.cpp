// ------------------------------------------------------------------------- \\
//								TEST PROGRAM								 \\
// ------------------------------------------------------------------------- \\
// Purpose:																	 \\
// This program tests the basic m_consumer and producer functionality,		 \\
// via sending packets back and forth collecting a ping latency.			 \\
// ------------------------------------------------------------------------- \\
// Author(s):																 \\
// Brandon J. Smith - July 4th, 2025										 \\
// ------------------------------------------------------------------------- \\
// License:																	 \\
// This file is part of the "Test Program" project, distributed under the	 \\
// MIT License.																 \\
// See the LICENSE file in the project root for more information.			 \\
// ------------------------------------------------------------------------- \\

#include <kafkalib.hpp>
#include <rdkafkacpp.h>
#include <chrono>

#define ASSERT(condition, message) \
	if (!(condition)) \
	{ \
		fprintf(stderr, "Assertion failed: %s\n", message); \
		exit(EXIT_FAILURE); \
	}

static void _delivery_report_callback(const RdKafka::Message &message)
{
	if (message.err() != RdKafka::ErrorCode::ERR_NO_ERROR) 
	{
		fprintf(stderr, "Message delivery failed: %s\n", message.errstr().c_str());
	} 
	/*else 
	{
		fprintf(stderr, "Message delivered successfully to %s [%d] at offset %lld\n",
				message.topic_name().c_str(), message.partition(), message.offset());
	}*/
}

static void _logger_callback(const RdKafka::Severity level, const std::string &message)
{
	switch (level) 
	{
		case RdKafka::Event::Severity::EVENT_SEVERITY_DEBUG:
			fprintf(stderr, "> DEBUG: %s\n", message.c_str());
			break;
		case RdKafka::Event::Severity::EVENT_SEVERITY_INFO:
			fprintf(stderr, "> INFO: %s\n", message.c_str());
			break;
		case RdKafka::Event::Severity::EVENT_SEVERITY_NOTICE:
			fprintf(stderr, "> NOTICE: %s\n", message.c_str());
			break;
		case RdKafka::Event::Severity::EVENT_SEVERITY_WARNING:
			fprintf(stderr, "> WARNING: %s\n", message.c_str());
			break;
		case RdKafka::Event::Severity::EVENT_SEVERITY_ERROR:
			fprintf(stderr, "> ERROR: %s\n", message.c_str());
			break;
		default:
			fprintf(stderr, "> UNKNOWN: %s\n", message.c_str());
			break;
	}
}

static void _rebalance_callback(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err, std::vector<RdKafka::TopicPartition*> &partitions) 
{
	if (err == RdKafka::ErrorCode::ERR__ASSIGN_PARTITIONS) 
	{
		fprintf(stderr, "Rebalance: Assigning partitions\n");
		consumer->assign(partitions); // Assign the partitions to the consumer.
	} 
	else if (err == RdKafka::ErrorCode::ERR__REVOKE_PARTITIONS) 
	{
		fprintf(stderr, "Rebalance: Revoking partitions\n");
		consumer->unassign(); // Unassign the partitions from the consumer.
	} 
	else 
	{
		fprintf(stderr, "Rebalance error: %s\n", RdKafka::err2str(err).c_str());
	}
}

int main() 
{
	GodotStreaming::KafkaController controller;

	// Initialize a Publisher...
	GodotStreaming::KafkaPublisherMetadata publisherMetadata;
	publisherMetadata.brokers = "localhost:19092";
	publisherMetadata.severity_log_level = RdKafka::Severity::EVENT_SEVERITY_DEBUG;
	publisherMetadata.delivery_report_callback = _delivery_report_callback;
	publisherMetadata.logger_callback = _logger_callback;
	//publisherMetadata.flush_immediately = true;

	GodotStreaming::Result<GodotStreaming::KafkaPublisher> publisher = controller.CreatePublisher(publisherMetadata);
	if (!publisher) 
	{
		fprintf(stderr, "Failed to create publisher: %s\n", publisher.error_message.c_str());
		return 1;
	}

	// Initialize a Consumer...
	GodotStreaming::KafkaSubscriberMetadata consumerMetadata;
	consumerMetadata.brokers = "127.0.0.1:19092";
	consumerMetadata.topics = std::vector<std::string>{"test_topic"};
	consumerMetadata.group_id = "test_group";
	consumerMetadata.severity_log_level = RdKafka::Severity::EVENT_SEVERITY_DEBUG;
	consumerMetadata.logger_callback = _logger_callback;
	consumerMetadata.delivery_report_callback = _delivery_report_callback;
	consumerMetadata.rebalance_callback = _rebalance_callback;
	consumerMetadata.offset_reset = RdKafka::OffsetSpec_t::OFFSET_END;
	consumerMetadata.enable_auto_commit = true;
	consumerMetadata.enable_partition_eof = true;

	GodotStreaming::Result<GodotStreaming::KafkaSubscriber> consumer = controller.CreateConsumer(consumerMetadata);
	if (!consumer) 
	{
		fprintf(stderr, "Failed to create consumer: %s\n", consumer.error_message.c_str());
		return 1;
	}

	GodotStreaming::KafkaPublisher &kafkaPublisher = *publisher.value;
	GodotStreaming::KafkaSubscriber &kafkaConsumer = *consumer.value;
	
	do
	{
		// In a real application, you would likely have a loop here to continuously send packets.
		// Get the current now.
		auto now = std::chrono::system_clock::now();
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		// Create a packet and serialize the current time as a timestamp.
		GodotStreaming::Packet packet;
		packet.Serialize<int64_t>(now_ms); // Serialize the current time as a timestamp.

		GodotStreaming::Status sent_status = kafkaPublisher.Publish(consumerMetadata.topics[0], packet, std::to_string(now_ms));

		if (!sent_status) 
		{
			fprintf(stderr, "Failed to send packet: %s\n", sent_status.message.c_str());
			return 1;
		}

		// Now, we will consume the packet and read the timestamp.
		std::vector<GodotStreaming::Packet> packets;
		GodotStreaming::Status poll_status = kafkaConsumer.Poll(packets, 15, 1);
		if (!poll_status) 
		{
			fprintf(stderr, "Polling failed: %s\n", poll_status.message.c_str());
			continue; // Polling failed, continue to the next iteration.
		}
		if (packets.empty()) 
		{
			continue; // No packets received, continue to the next iteration.
		}

		// Assuming we received at least one packet, we can read the timestamp.
		GodotStreaming::Packet &received_packet = packets[packets.size() - 1];
		GodotStreaming::Result<int64_t> received_timestamp = received_packet.Deserialize<int64_t>();
		if (!received_timestamp) 
		{
			fprintf(stderr, "Failed to deserialize packet: %s\n", received_timestamp.error_message.c_str());
			return 1;
		}
		int64_t received_time = *received_timestamp.value;

		if (now_ms == received_time)
		{
			// Calculate the latency.
			auto post_now = std::chrono::system_clock::now();
			auto latency = post_now - std::chrono::system_clock::time_point(std::chrono::milliseconds(received_time));

			auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(latency).count();
			fprintf(stderr, "Ping latency: %lld ms, %lld | %lld | %lld\n", latency_ms, now_ms, received_time, packets.size());
			ASSERT(latency_ms >= 0, "Latency should not be negative."); // If this happens, clean your Kafka topic; if it continues, there's a serious issue with this.
		}

	} while (true);

	return 0;
}