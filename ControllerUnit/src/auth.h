#ifndef AUTH_H
#define AUTH_H
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "auth_secrets.h"

String JWT_signUp();
String JWT_signIn(const char *email, const char *password);

/**
 * @brief Structure to hold HTTP response details
 * @param statusCode HTTP status code
 * @param payload Response body as a String
 */
struct HttpResponse
{
    int statusCode;
    String payload;
};

// Helper: Send POST request and parse response
HttpResponse sendPostRequest(String url, String jsonBody)
{
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(jsonBody);
    String payload = http.getString();
    http.end();
    return {httpCode, payload};
}

// Sign-up function
String JWT_signUp()
{
    JsonDocument doc;
    doc["email"] = userEmail;
    doc["name"] = userName;
    doc["password"] = userPassword;
    doc["role"] = userRole;
    doc["companyName"] = userCompanyName;
    String requestBody;
    serializeJson(doc, requestBody);

    HttpResponse response = sendPostRequest(String(AuthUrl) + "sign-up", requestBody);

    if (response.statusCode == 201)
    {
        // Success - parse and return JWT token
        JsonDocument respDoc;
        DeserializationError err = deserializeJson(respDoc, response.payload);
        if (!err && respDoc["token"].is<String>())
        {
            return respDoc["token"].as<String>();
        }
        // Token not found in response
        return "";
    }
    else if (response.statusCode == 400)
    {
        // Invalid request body
        Serial.println("Sign-up failed: Invalid request body");
        return "";
    }
    else if (response.statusCode == 409)
    {
        // Email or name already taken - try sign-in instead
        String signInResult = JWT_signIn(userEmail, userPassword);
        if (!signInResult.isEmpty())
        {
            return signInResult;
        }
        return "";
    }
    else if (response.statusCode == 500)
    {
        // Internal server error
        Serial.println("Sign-up failed: Internal server error");
        return "";
    }
    else
    {
        // Other error codes
        Serial.printf("Sign-up failed with status code: %d\n", response.statusCode);
        return "";
    }
}

// Sign-in function
String JWT_signIn(const char *email, const char *password)
{
    JsonDocument doc;
    doc["email"] = email;
    doc["password"] = password;
    String requestBody;
    serializeJson(doc, requestBody);

    HttpResponse response = sendPostRequest(String(AuthUrl) + "sign-in", requestBody);

    if (response.statusCode == 200)
    {
        // Success - parse and return JWT token
        JsonDocument respDoc;
        DeserializationError err = deserializeJson(respDoc, response.payload);
        if (!err && respDoc["token"].is<String>())
        {
            return respDoc["token"].as<String>();
        }
    }
    else if (response.statusCode == 401)
    {
        // Unauthorized - invalid credentials
        Serial.println("Sign-in failed: Invalid credentials");
    }
    else if (response.statusCode == 400)
    {
        // Bad request
        Serial.println("Sign-in failed: Bad request");
    }
    else
    {
        // Other error codes
        Serial.printf("Sign-in failed with status code: %d\n", response.statusCode);
    }
    return "";
}

#endif // AUTH_H