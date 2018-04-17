/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file subscribe_publish_cpp_sample.cpp
 * @brief simple MQTT publish and subscribe on the same topic in C++
 *
 * This example takes the parameters from the aws_iot_config.h file and establishes a connection to the AWS IoT MQTT Platform.
 * It subscribes and publishes to the same topic - "sdkTest/sub"
 *
 * If all the certs are correct, you should see the messages received by the application in a loop.
 *
 * The application takes in the certificate path, host name , port and the number of times the publish should happen.
 *
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

#ifdef MBEDTLS_SE
#include "CinterionModem.h"
#include "mbedtls_se.h"

static CinterionModem modem;
#endif

const char AWS_IOT_ROOT_CA[] = "-----BEGIN CERTIFICATE-----\n"
"MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\n"
"yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
"ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
"U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
"ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
"aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\n"
"MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
"ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\n"
"biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\n"
"U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\n"
"aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\n"
"nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\n"
"t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\n"
"SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\n"
"BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\n"
"rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\n"
"NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\n"
"BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\n"
"BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\n"
"aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\n"
"MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\n"
"p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\n"
"5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\n"
"WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\n"
"4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\n"
"hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\n"
"-----END CERTIFICATE-----\n";

#ifndef MBEDTLS_SE

const char AWS_IOT_CERTIFICATE[] = "-----BEGIN CERTIFICATE-----\n"
"MIIDWTCCAkGgAwIBAgIUNbymoXV+EfYUfXamM7BEQQLzdVcwDQYJKoZIhvcNAQEL\n"
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTE3MDQyMTA5Mjc0\n"
"M1oXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAIlw1XYxR49Xh3AlVIoM\n"
"ngjJfnHxA0mcjFfTWlAYGzVW6sbjj3TFUD2LsDr+CP19vmZ/YIGQm40OBp8MAZiF\n"
"kQPdf/v9ufj6iZQtsFhhGozrWFZu+31L2kAFVh46n8YwlvaQ2SLa7AJZsXxudNZy\n"
"qh/LlqAKWE2TDQkdIYEU3r+yrnwXQv9g2uhmI/1UN6EFsLXQ+PDW28fwevQnpZPz\n"
"ADbizT9Jn0uAdSVHH3KQjSiQmeZyPewiafG/yWsmXzNcJAaUPcdPtKxg3abdVqlx\n"
"laKq+mkScxW3s2Vq/89ruC667ysru3mqIUj2P67UPyyeXprenWcvpkfmQqIxw+3p\n"
"XVECAwEAAaNgMF4wHwYDVR0jBBgwFoAUpLWiKww2qpRXzG+dySYBAzWoR+UwHQYD\n"
"VR0OBBYEFO/R17ikgXwCx1Hu0uH5voqcux2YMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAWE+mkOFVD5uvwto/62m1r6cJN\n"
"QZ5UG+0TnDG2LJYsEnxvLbqOMAMpCc3gZBN6gd7ArimLXPPZut451Dxk2GaeDN0Z\n"
"rK/aQgU6LUrT8kDnie3AXC9aeZ0RgP88N6k7YosqdPGM3ExLoZnFfnxFHBdkQxpL\n"
"znjAZDK7Zh0ZlaNahIq79OpGoPCaMJcdcbg8ocSfp3vdXD2/a2yvVWpHecISkAf8\n"
"DYYgYYFmFu8Abj82MLsejR57udxVkNRfkY6AUw+gHwJ7+IbaFZ+WZ17PZ9eMuWfB\n"
"d6XNwUMUenUisboTt2xxI3AFTjCGI/NYApb2x5zRtT+XR131wQPjCuc63FrI\n"
"-----END CERTIFICATE-----\n";

const char AWS_IOT_PRIVATE_KEY[] = "-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEAiXDVdjFHj1eHcCVUigyeCMl+cfEDSZyMV9NaUBgbNVbqxuOP\n"
"dMVQPYuwOv4I/X2+Zn9ggZCbjQ4GnwwBmIWRA91/+/25+PqJlC2wWGEajOtYVm77\n"
"fUvaQAVWHjqfxjCW9pDZItrsAlmxfG501nKqH8uWoApYTZMNCR0hgRTev7KufBdC\n"
"/2Da6GYj/VQ3oQWwtdD48Nbbx/B69Celk/MANuLNP0mfS4B1JUcfcpCNKJCZ5nI9\n"
"7CJp8b/JayZfM1wkBpQ9x0+0rGDdpt1WqXGVoqr6aRJzFbezZWr/z2u4LrrvKyu7\n"
"eaohSPY/rtQ/LJ5emt6dZy+mR+ZCojHD7eldUQIDAQABAoIBAQCEQjqIZF+yhs5k\n"
"kufJeN7TIeM2DnHVcnpzQYXVMX1tVNjUz8MK4Z2XvEa4XiGUnl6ND+J0jA3ELU0d\n"
"3FXkyhOEnrA8dCt0dtPR7i2Wvsrn9MmrU11bc5TwqrH2oP+DntqHJ1hsfDNFB78O\n"
"ONTiULF5q1alZ48WB34x2QCW0NiRqx3LRGddX+sXTj4F/X30375EPmCNXJobl+N+\n"
"i0+7PiiavLj/1ehOOk/vIm7/QU9DZD4vfdU4oEkibdx2RtYc6SoQrdwHHCRhJenn\n"
"3gNwHKTwvMax2cQcb4Hxka/ep6VIjx1APCWJm9U2hBUhKVdgsg/fbuhw7bt5XGmT\n"
"GISLxjQBAoGBAL0L/kd3sqlhLkyNnpq9YQzvYEp88l87Z3luP3qLsB8HA2Hlf1WO\n"
"gO7Wh0NPbjZQGyBkfMbsCMmoZEyhGpo3F/B8pQOfhHIe2MqGi081BRa28vvRRETB\n"
"f7Jx3uSWNlOvhosy1/ZWjfhwV8ZU1mQmjiWLZt2FgK1W5NIYJUUpAkLhAoGBALod\n"
"9DpFP/V4iNWPX5scG73HWMkZPrm+yeBVPdEiVIN62YCFfxPvwq8hK7rHmNey/n3u\n"
"7MyuUx0xz0s/7dwCrMFYTGRX3vUdErvrHs/9I3+OeJVXrPfrCYxZcZ42t9lBiw5Y\n"
"2atH4ZLCKHcbFGHUtI4J4Qv148HUKT+zUqHW/NhxAoGAZ8FqWM0gOIhGwetELkdc\n"
"OZ4zqg4zdAMFgob4vghmrFDMEXHE1i81ImCJsm6o3ZRPnxKnzQGEvTD3g3s0P4mX\n"
"UP0IEBn/tiap81WupdVCqrnUWFL9dgMBU+3dWHX88Sc337QTBXdxyfXWptqvJB5p\n"
"C8Abv97ixcAYLBrmcbPVMuECgYEArXiV1moNPyJlh4fmuI/uW2ienHFnQYFOcEWD\n"
"JJY254VtjJjg5RgoAUuNkr3O+9zdz0sOc4hX93IhoCWp8dKcaml+alhse1Hp0DVJ\n"
"ttpeZ9nHEotxsHHlqGcFu02M9nZcwbaWy0poOX22ca1Pxg+XanxSwe8hpdu1xqXB\n"
"cw8LBEECgYAy/50jv/vWrf7ZZO7YA+RrY9gB29YOmhFMS9KcAoIHj+WV+YPdq4JR\n"
"oaj6Rj9JP9JTBzK8aqJeTQ80SXe+53igoNOal16V3CM7V/tiTHKrFjVo0iGb48Uj\n"
"iLpNxh7EA3jggrGqnstnYSWcHqSSX0Spxn9uNyY5bltflKy0ag6MtQ==\n"
"-----END RSA PRIVATE KEY-----\n";

#endif

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;

/**
 * @brief This parameter will avoid infinite loop of publish and exit the program after certain number of publishes
 */
uint32_t publishCount = 0;

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
									IoT_Publish_Message_Params *params, void *pData) {
	IOT_UNUSED(pData);
	IOT_UNUSED(pClient);
	IOT_INFO("Subscribe callback");
	IOT_INFO("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, params->payload);
}

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
	IOT_WARN("MQTT Disconnect");
	IoT_Error_t rc = FAILURE;

	if(NULL == pClient) {
		return;
	}

	IOT_UNUSED(data);

	if(aws_iot_is_autoreconnect_enabled(pClient)) {
		IOT_INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
	} else {
		IOT_WARN("Auto Reconnect not enabled. Starting manual reconnect...");
		rc = aws_iot_mqtt_attempt_reconnect(pClient);
		if(NETWORK_RECONNECTED == rc) {
			IOT_WARN("Manual Reconnect Successful");
		} else {
			IOT_WARN("Manual Reconnect Failed - %d", rc);
		}
	}
}

void intHandler(int dummy) {
	IOT_INFO("\nBye bye\n");
	#ifdef MBEDTLS_SE
	modem.close();
	#endif
	exit(0);
}

int main(int argc, char **argv) {
	bool infinitePublishFlag = true;
	char cPayload[100];

	int32_t i = 0;

	IoT_Error_t rc = FAILURE;

	AWS_IoT_Client client;
	
	signal(SIGINT, intHandler);
	signal(SIGTERM, intHandler);
	
	IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
	IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

	IoT_Publish_Message_Params paramsQOS0;
	IoT_Publish_Message_Params paramsQOS1;

	IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
	
	#ifdef MBEDTLS_SE
	if(!modem.open()) {
		IOT_ERROR("\nError modem not found!\n");
		return -1;
	}
	mbedtls_se_init(&modem);
	#endif

        IOT_INFO("Setting initial init params");
        IOT_INFO("enableAutoReconnect = false");
        IOT_INFO("pHostURL = %s", HostAddress);
        IOT_INFO("port = %d", port);
        IOT_INFO("pRootCALocation = %s", AWS_IOT_ROOT_CA_FILENAME);
        IOT_INFO("pDeviceCertLocation = %s", AWS_IOT_CERTIFICATE_FILENAME);
        IOT_INFO("pDevicePrivateKeyLocation = %s", AWS_IOT_PRIVATE_KEY_FILENAME);
	mqttInitParams.enableAutoReconnect = false; // We enable this later below
	mqttInitParams.pHostURL = HostAddress;
	mqttInitParams.port = port;
	mqttInitParams.pRootCALocation = (char*) AWS_IOT_ROOT_CA_FILENAME;
	mqttInitParams.pDeviceCertLocation = (char*) AWS_IOT_CERTIFICATE_FILENAME;
	mqttInitParams.pDevicePrivateKeyLocation = (char*) AWS_IOT_PRIVATE_KEY_FILENAME;
	mqttInitParams.mqttCommandTimeout_ms = 20000;
	mqttInitParams.tlsHandshakeTimeout_ms = 5000;
	mqttInitParams.isSSLHostnameVerify = true;
	mqttInitParams.disconnectHandler = disconnectCallbackHandler;
	mqttInitParams.disconnectHandlerData = NULL;
	
	#ifdef MBEDTLS_SE
	//IOT_DEBUG("rootCA %s", mqttInitParams.pRootCALocation);
	IOT_INFO("clientCRT %s", mqttInitParams.pDeviceCertLocation);
	IOT_INFO("clientKey %s", mqttInitParams.pDevicePrivateKeyLocation);
	#endif

        IOT_INFO("Initializing MQTT with connections params\n");
	rc = aws_iot_mqtt_init(&client, &mqttInitParams);
	if(SUCCESS != rc) {
		IOT_ERROR("aws_iot_mqtt_init returned error : %d ", rc);
		return rc;
	}

	connectParams.keepAliveIntervalInSec = 10;
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = (char*) AWS_IOT_MQTT_CLIENT_ID;
	connectParams.clientIDLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);
	connectParams.isWillMsgPresent = false;

	IOT_INFO("Connecting...");
	rc = aws_iot_mqtt_connect(&client, &connectParams);
	IOT_INFO("After connecting");
	if(SUCCESS != rc) {
		IOT_ERROR("Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
		return rc;
	}
	IOT_INFO("Connecting complete! setting auto-reconnect");
	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
	if(SUCCESS != rc) {
		IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
		return rc;
	}

	IOT_INFO("Subscribing...");
	rc = aws_iot_mqtt_subscribe(&client, "RPI/Test", 8, QOS0, iot_subscribe_callback_handler, NULL);
	if(SUCCESS != rc) {
		IOT_ERROR("Error subscribing : %d ", rc);
		return rc;
	}

	sprintf(cPayload, "{\"message\":\"%s\",\"num\": %d}", "Hello from Raspberry Pi!", i);

	paramsQOS0.qos = QOS0;
	paramsQOS0.payload = (void *) cPayload;
	paramsQOS0.isRetained = 0;

	paramsQOS1.qos = QOS1;
	paramsQOS1.payload = (void *) cPayload;
	paramsQOS1.isRetained = 0;

	if(publishCount != 0) {
		infinitePublishFlag = false;
	}

	while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)
		  && (publishCount > 0 || infinitePublishFlag)) {

		//Max time the yield function will wait for read messages
		rc = aws_iot_mqtt_yield(&client, 100);
		if(NETWORK_ATTEMPTING_RECONNECT == rc) {
			// If the client is attempting to reconnect we will skip the rest of the loop.
			continue;
		}

		IOT_INFO("-->sleep");
		sleep(1);
		//sprintf(cPayload, "%s : %d ", "Hello from Raspberry Pi (QSO)!", i++);
	        sprintf(cPayload, "{\"message\":\"%s\",\"num\": %d}", "Hello from Raspberry Pi! (QS0)!", i++);
                IOT_INFO("Sending %s", cPayload);
		paramsQOS0.payloadLen = strlen(cPayload);
		rc = aws_iot_mqtt_publish(&client, "RPI/Test", 8, &paramsQOS0);
		if(publishCount > 0) {
			publishCount--;
		}

		//sprintf(cPayload, "%s : %d ", "Hello from Raspberry Pi (QS1)!", i++);
	        sprintf(cPayload, "{\"message\":\"%s\",\"num\": %d}", "Hello from Raspberry Pi! (QS1)!", i++);
		paramsQOS1.payloadLen = strlen(cPayload);
                IOT_INFO("Sending %s", cPayload);
		rc = aws_iot_mqtt_publish(&client, "RPI/Test", 8, &paramsQOS1);
		if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
			IOT_WARN("QOS1 publish ack not received.\n");
			rc = SUCCESS;
		}
		if(publishCount > 0) {
			publishCount--;
		}
	}

	if(SUCCESS != rc) {
		IOT_ERROR("An error occurred in the loop.\n");
	} else {
		IOT_INFO("Publish done\n");
	}
	
	#ifdef MBEDTLS_SE
	modem.close();
	#endif

	return rc;
}
