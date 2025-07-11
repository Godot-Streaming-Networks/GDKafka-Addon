Godot Kafka (GDKafka) Plugin
=======================================

This plugin provides a Godot interface to Kafka, allowing you to produce and consume messages in a Kafka cluster directly from your Godot projects.

It is designed to be easy to use and integrate into your Godot applications, enabling real-time data processing and communication.

# Available Platforms
✅ Windows  
✅ Linux
❌ MacOSX - (Author doesn't have a macOS machine to test on, but it should work)
❌ Android - Not planned
❌ iOS - Not planned
❌ HTML5 (WebAssembly) - Not planned

# Supported Godot Versions
✅ Godot 4.4
⚙ Godot 4.4 (Mono) - (Planned, but not yet implemented)
❌ Godot <4.3 - (Not supported)
❌ Godot 3.x - (Not supported)

## Usages
* Producing messages to Kafka topics.
* Consuming messages from Kafka topics.
* Handling Kafka message serialization and deserialization.

## Target Audience
This plugin is intended for Godot developers who need to integrate Kafka messaging capabilities into their games or applications. Projects like making a Massive Multiplayer Online Game (MMO) or any application that requires real-time data exchange can benefit from this plugin.

## Best Practices
* Ideally this plugin should be used in a dedicated server/service - not in the game client. ( It can be used in the client, but it is not recommended. )
* Deploy your Kafka brokers ideally on separate bare-metal machines for maximum performance.
    * Ensure that your Kafka cluster is properly configured and secured.

## Not Recommended For
* This plugin is not recommended for use in the Godot editor itself, as it is designed for runtime use in games or applications.
* It is not recommended to use this plugin for small-scale projects or applications that do not require real-time messaging capabilities.
* Avoid using this plugin in scenarios where low latency is critical, as network latency can affect message delivery times.
* When using this plugin in a client application, be aware of the potential security implications of exposing Kafka brokers directly to the client - consider using a HTTPS REST API or WebSocket connection to interact with a backend service that may handle Kafka interactions instead.

## Known Services
* [Redpanda](https://redpanda.com/) - A Kafka-compatible streaming platform that can be used as a drop-in replacement for Kafka; re-written in C++ for high performance and low latency.
* [Apache Kafka](https://kafka.apache.org/) - The original Kafka service, widely used for real-time data streaming and messaging.
* [AWS MSK (Managed Streaming for Kafka)](https://aws.amazon.com/msk/) - A fully managed service that makes it easy to build and run applications that use Apache Kafka.
* [Azure Event Hubs](https://azure.microsoft.com/en-us/services/event-hubs/) - A fully managed, real-time data ingestion service that can be used as a Kafka-compatible service.
* [Google Cloud Pub/Sub](https://cloud.google.com/pubsub) - A messaging service that can be used for real-time messaging and data streaming, with Kafka compatibility.
* [Confluent Cloud](https://www.confluent.io/confluent-cloud/) - A fully managed Kafka service that provides a cloud-native experience for building real-time applications.
* [Aiven Kafka](https://aiven.io/kafka) - A fully managed Apache Kafka service that provides a cloud-native experience for building real-time applications.
* [Instaclustr Managed Kafka](https://www.instaclustr.com/platform/managed-apache-kafka/) - A fully managed Apache Kafka service that provides a cloud-native experience for building real-time applications.
* [Bitnami Kafka](https://bitnami.com/stack/kafka) - A fully managed Apache Kafka service that provides a cloud-native experience for building real-time applications.
* [IBM Event Streams](https://www.ibm.com/cloud/event-streams) - A fully managed Apache Kafka service that provides a cloud-native experience for building real-time applications.
* [StreamNative Cloud](https://streamnative.io/cloud) - A fully managed Apache Pulsar service that provides a cloud-native experience for building real-time applications, with Kafka compatibility.

## Contributing
Contributions to the GDKafka plugin are welcome! If you have suggestions, bug reports, or feature requests, please open an issue on the [GitHub repository](https://github.com/Godot-Streaming-Networks/GDKafka-Addon)

## License
This plugin is licensed under the MIT License, which allows for free use, modification, and distribution of the code. For more details, please refer to the [LICENSE](LICENSE) file in the repository.