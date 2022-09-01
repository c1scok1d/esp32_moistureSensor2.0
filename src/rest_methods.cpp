#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WString.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "rest_methods.h"

#define MAX_HTTP_OUTPUT_BUFFER_POST 1024 * 2
// No changes made in this handler
esp_err_t _http_event_handler_post(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        //printf( "HTTP_EVENT_ERROR\n");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        //printf( "HTTP_EVENT_ON_CONNECTED\n");
        break;
    case HTTP_EVENT_HEADER_SENT:
        //printf( "HTTP_EVENT_HEADER_SENT\n");
        break;
    case HTTP_EVENT_ON_HEADER:
       // printf( "HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        printf( "HTTP_EVENT_ON_DATA, len=%d\n", evt->data_len);
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            if (evt->user_data)
            {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            }
            else
            {
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        printf("Failed to allocate memory for output buffer\n");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }
            output_len += evt->data_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        printf( "HTTP_EVENT_ON_FINISH\n");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        printf("HTTP_EVENT_DISCONNECTED\n");
        int mbedtls_err = 0;
        // esp_err_t err = esp_tls_get_and_clear_last_error(NULL,evt->data, &mbedtls_err, NULL);
        // if (err != 0)
        // {
        //     if (output_buffer != NULL)
        //     {
        //         free(output_buffer);
        //         output_buffer = NULL;
        //     }
        //     output_len = 0;
        //     ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
        //     ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        // }
        break;
    }
    return ESP_OK;
}



int POST(String server_uri, String to_send)
{
    printf(server_uri.c_str());
    printf(to_send.c_str());
    printf("\nPOST\n");
    char local_response_buffer_post[MAX_HTTP_OUTPUT_BUFFER_POST] = {0};

    esp_http_client_handle_t http_client_post; // its being used in the get post and put method files

    esp_http_client_config_t config = {
        .host = "", // Setting these in individual GET POST and PUT request
        .path = "",
        .event_handler = _http_event_handler_post,
        .user_data = local_response_buffer_post, // Pass address of local buffer to get response
    };

    http_client_post = esp_http_client_init(&config);

    // printf( "%s", to_send);

    esp_http_client_set_url(http_client_post, server_uri.c_str());
    esp_http_client_set_method(http_client_post, HTTP_METHOD_POST);
    esp_http_client_set_header(http_client_post, "Content-Type", "application/x-www-form-urlencoded");

    esp_http_client_set_post_field(http_client_post, to_send.c_str(), strlen(to_send.c_str()));

    esp_err_t err = esp_http_client_perform(http_client_post);

    int status_code = esp_http_client_get_status_code(http_client_post);

    esp_http_client_cleanup(http_client_post);

    if (err == ESP_OK)
    {
        printf("Success POST\n");
        printf("Status code %d\n", status_code);
        return status_code;
    }
    else
    {
        printf("HTTP POST request failed: %s\n");
        return status_code;
    }
}
