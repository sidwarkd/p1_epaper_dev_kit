/* HTTPClient.cpp */
/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef UTILITY_H
#include <stdlib.h>
#define swMalloc malloc         // use the standard
#define swFree free
#endif

#include <cstdio>

#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

#define CHUNK_SIZE 512
#define MAXLEN_VALUE 160    /* Max URL and Max Value for Name:Value */

#include <cstring>

#include "HTTPClient.h"

HTTPClient::HTTPClient(TCPClient *client) : 
    m_client(client),
    m_basicAuthUser(NULL), m_basicAuthPassword(NULL),
    m_nCustomHeaders(0), m_httpResponseCode(0),
    m_maxredirections(1), m_location(NULL)
{

}

HTTPClient::~HTTPClient()
{
    if (m_basicAuthUser)
        swFree(m_basicAuthUser);
    if (m_basicAuthPassword)
        swFree(m_basicAuthPassword);
    if (m_location) // if any redirection was involved, clean up after it.
        swFree(m_location);
    m_location = NULL;      // this step isn't necessary...
}


const char * HTTPClient::GetErrorMessage(HTTPResult res)
{
    const char * msg[HTTP_CLOSED+1] = {
        "HTTP OK",            ///<Success
        "HTTP Processing",    ///<Processing
        "HTTP URL Parse error",         ///<url Parse error
        "HTTP DNS error",           ///<Could not resolve name
        "HTTP Protocol error",         ///<Protocol error
        "HTTP 404 Not Found",      ///<HTTP 404 Error
        "HTTP 403 Refused",       ///<HTTP 403 Error
        "HTTP ### Error",         ///<HTTP xxx error
        "HTTP Timeout",       ///<Connection timeout
        "HTTP Connection error",          ///<Connection error
        "HTTP Closed by remote host"         ///<Connection was closed by remote host
    };
    if (res <= HTTP_CLOSED)
        return msg[res];
    else
        return "HTTP Unknown Code";
};


void HTTPClient::basicAuth(const char* user, const char* password) //Basic Authentification
{
    if (m_basicAuthUser)
        swFree(m_basicAuthUser);
    m_basicAuthUser = (char *)swMalloc(strlen(user)+1);
    strcpy(m_basicAuthUser, user);
    if (m_basicAuthPassword)
        swFree(m_basicAuthPassword);
    m_basicAuthPassword = (char *)swMalloc(strlen(password)+1);
    strcpy(m_basicAuthPassword, password);
}

void HTTPClient::customHeaders(const char **headers, size_t pairs)
{
    m_customHeaders = headers;
    m_nCustomHeaders = pairs;
}


HTTPResult HTTPClient::get(const char* url, IHTTPDataIn* pDataIn, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
    Serial.printlnf("url: %s, timeout: %d", url, timeout);
    return connect(url, HTTP_GET, NULL, pDataIn, timeout);
}

HTTPResult HTTPClient::get(const char* url, char* result, size_t maxResultLen, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
    Serial.printlnf("url: %s, maxResultLen %d, timeout: %d", url, maxResultLen, timeout);
    HTTPText str(result, maxResultLen);
    return get(url, &str, timeout);
}

HTTPResult HTTPClient::post(const char* url, const IHTTPDataOut& dataOut, IHTTPDataIn* pDataIn, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
    return connect(url, HTTP_POST, (IHTTPDataOut*)&dataOut, pDataIn, timeout);
}

HTTPResult HTTPClient::put(const char* url, const IHTTPDataOut& dataOut, IHTTPDataIn* pDataIn, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
    return connect(url, HTTP_PUT, (IHTTPDataOut*)&dataOut, pDataIn, timeout);
}

HTTPResult HTTPClient::del(const char* url, IHTTPDataIn* pDataIn, int timeout /*= HTTP_CLIENT_DEFAULT_TIMEOUT*/) //Blocking
{
    return connect(url, HTTP_DELETE, NULL, pDataIn, timeout);
}


int HTTPClient::getHTTPResponseCode()
{
    return m_httpResponseCode;
}

void HTTPClient::setMaxRedirections(int i)
{
    if (i < 1)
        i = 1;
    m_maxredirections = i;
}

#define CHECK_CONN_ERR(ret) \
  do{ \
    if(ret) { \
      m_client->stop(); \
      Serial.printlnf("Connection error (%d)", ret); \
      return HTTP_CONN; \
    } \
  } while(0)

#define PRTCL_ERR() \
  do{ \
    m_client->stop(); \
    Serial.println("Protocol error"); \
    return HTTP_PRTCL; \
  } while(0)

HTTPResult HTTPClient::connect(const char* url, HTTP_METH method, IHTTPDataOut* pDataOut, IHTTPDataIn* pDataIn, int timeout) //Execute request
{
    m_httpResponseCode = 0; //Invalidate code
    m_timeout = timeout;

    Serial.printlnf("connect(%s,%d,...,%d)", url, method, timeout);
    pDataIn->writeReset();
    if( pDataOut ) {
        pDataOut->readReset();
    }

    char scheme[8];
    uint16_t port;
    char host[32];
    char path[MAXLEN_VALUE];
    size_t recvContentLength = 0;
    bool recvChunked = false;
    size_t crlfPos = 0;
    char buf[CHUNK_SIZE];
    size_t trfLen;
    int ret = 0;

    int maxRedirect = m_maxredirections;

    while (maxRedirect--) {
        bool takeRedirect = false;

        Serial.printlnf("parse: [%s]\n", url);
        //First we need to parse the url (http[s]://host[:port][/[path]]) -- HTTPS not supported (yet?)
        HTTPResult res = parseURL(url, scheme, sizeof(scheme), host, sizeof(host), &port, path, sizeof(path));
        if(res != HTTP_OK) {
            Serial.printlnf("parseURL returned %d", res);
            return res;
        }

        if(port == 0) { //TODO do handle HTTPS->443
            port = 80;
        }

        Serial.printlnf("Scheme: %s", scheme);
        Serial.printlnf("Host: %s", host);
        Serial.printlnf("Port: %d", port);
        Serial.printlnf("Path: %s", path);

        //Connect
        Serial.println("Connecting socket to server");
        ret = m_client->connect(host, port);
        if (!ret) {
            m_client->stop();
            Serial.println("Could not connect");
            return HTTP_CONN;
        }

        // Send request
        Serial.println("Sending request");
        const char* meth = (method==HTTP_GET)?"GET":(method==HTTP_POST)?"POST":(method==HTTP_PUT)?"PUT":(method==HTTP_DELETE)?"DELETE":"";
        snprintf(buf, sizeof(buf), "%s %s HTTP/1.1\r\nHost: %s:%d\r\n", meth, path, host, port); //Write request
        Serial.printlnf(" buf{%s}", buf);
        ret = send(buf);
        if (ret) {
            m_client->stop();
            Serial.println("Could not write request");
            return HTTP_CONN;
        }

        // send authorization
        Serial.println("send auth (if defined)");
        if (m_basicAuthUser && m_basicAuthPassword) {
            strcpy(buf, "Authorization: Basic ");
            createauth(m_basicAuthUser, m_basicAuthPassword, buf+strlen(buf), sizeof(buf)-strlen(buf));
            strcat(buf, "\r\n");
            Serial.printlnf(" (%s,%s) => (%s)", m_basicAuthUser, m_basicAuthPassword, buf);
            ret = send(buf);
            Serial.printlnf(" ret = %d", ret);
            if(ret) {
                m_client->stop();
                Serial.println("Could not write request");
                return HTTP_CONN;
            }
        }

        // Send all headers
        Serial.printlnf("Send custom header(s) %d (if any)", m_nCustomHeaders);
        for (size_t nh = 0; nh < m_nCustomHeaders * 2; nh+=2) {
            Serial.printlnf("hdr[%2d] %s: %s", nh, m_customHeaders[nh], m_customHeaders[nh+1]);
            snprintf(buf, sizeof(buf), "%s: %s\r\n", m_customHeaders[nh], m_customHeaders[nh+1]);
            ret = send(buf);
            if (ret) {
                Serial.println("closing");
                /*ATOMIC_BLOCK(){
                delayMicroseconds(50000);
                }*/
                m_client->stop();
                Serial.println("Could not write request");
                return HTTP_CONN;
            }
            Serial.printlnf("   send() returned %d", ret);
        }

        //Send default headers
        Serial.println("Sending headers");
        if( pDataOut != NULL ) {
            if( pDataOut->getIsChunked() ) {
                ret = send("Transfer-Encoding: chunked\r\n");
                CHECK_CONN_ERR(ret);
            } else {
                snprintf(buf, sizeof(buf), "Content-Length: %zu\r\n", pDataOut->getDataLen());
                ret = send(buf);
                CHECK_CONN_ERR(ret);
            }
            char type[48];
            if( pDataOut->getDataType(type, 48) == HTTP_OK ) {
                snprintf(buf, sizeof(buf), "Content-Type: %s\r\n", type);
                ret = send(buf);
                CHECK_CONN_ERR(ret);
            }
        }

        //Close headers
        Serial.println("Headers sent");
        ret = send("\r\n");
        CHECK_CONN_ERR(ret);

        //Send data (if available)
        if( pDataOut != NULL ) {
            Serial.printlnf("Sending data [%s]", buf);
            while(true) {
                size_t writtenLen = 0;
                pDataOut->read(buf, CHUNK_SIZE, &trfLen);
                Serial.printlnf("  trfLen: %d", trfLen);
                if( pDataOut->getIsChunked() ) {
                    //Write chunk header
                    char chunkHeader[16];
                    snprintf(chunkHeader, sizeof(chunkHeader), "%zu\r\n", trfLen); //In hex encoding
                    ret = send(chunkHeader);
                    CHECK_CONN_ERR(ret);
                } else if( trfLen == 0 ) {
                    break;
                }
                if( trfLen != 0 ) {
                    ret = send(buf, trfLen);
                    CHECK_CONN_ERR(ret);
                }

                if( pDataOut->getIsChunked()  ) {
                    ret = send("\r\n"); //Chunk-terminating CRLF
                    CHECK_CONN_ERR(ret);
                } else {
                    writtenLen += trfLen;
                    if( writtenLen >= pDataOut->getDataLen() ) {
                        break;
                    }
                }

                if( trfLen == 0 ) {
                    break;
                }
            }

        }

        //Receive response
        Serial.println("Receiving response");
        ret = recv(buf, 1, CHUNK_SIZE - 1, &trfLen);    // recommended by Rob Noble to avoid timeout wait
        CHECK_CONN_ERR(ret);
        buf[trfLen] = '\0';
        Serial.printlnf("Received \r\n(%s\r\n)", buf);

        char* crlfPtr = strstr(buf, "\r\n");
        if( crlfPtr == NULL) {
            PRTCL_ERR();
        }

        crlfPos = crlfPtr - buf;
        buf[crlfPos] = '\0';

        //Parse HTTP response
        if( sscanf(buf, "HTTP/%*d.%*d %d %*[^\r\n]", &m_httpResponseCode) != 1 ) {
            //Cannot match string, error
            Serial.printlnf("Not a correct HTTP answer : {%s}\n", buf);
            PRTCL_ERR();
        }

        if( (m_httpResponseCode < 200) || (m_httpResponseCode >= 400) ) {
            //Did not return a 2xx code; TODO fetch headers/(&data?) anyway and implement a mean of writing/reading headers
            Serial.printlnf("Response code %d", m_httpResponseCode);
            PRTCL_ERR();
        }

        Serial.println("Reading headers");

        memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2) + 1); //Be sure to move NULL-terminating char as well
        trfLen -= (crlfPos + 2);

        recvContentLength = 0;
        recvChunked = false;
        //Now get headers
        while( true ) {
            crlfPtr = strstr(buf, "\r\n");
            if(crlfPtr == NULL) {
                if( trfLen < CHUNK_SIZE - 1 ) {
                    size_t newTrfLen = 0;
                    ret = recv(buf + trfLen, 1, CHUNK_SIZE - trfLen - 1, &newTrfLen);
                    trfLen += newTrfLen;
                    buf[trfLen] = '\0';
                    Serial.printlnf("Read %d chars; In buf: [%s]", newTrfLen, buf);
                    CHECK_CONN_ERR(ret);
                    continue;
                } else {
                    PRTCL_ERR();
                }
            }

            crlfPos = crlfPtr - buf;

            if(crlfPos == 0) { //End of headers
                Serial.println("Headers read");
                memmove(buf, &buf[2], trfLen - 2 + 1); //Be sure to move NULL-terminating char as well
                trfLen -= 2;
                break;
            }

            buf[crlfPos] = '\0';

            char key[64];
            char value[MAXLEN_VALUE];

            key[63] = '\0';
            value[MAXLEN_VALUE - 1] = '\0';

            int n = sscanf(buf, "%63[^:]: %159[^\r\n]", key, value);
            if ( n == 2 ) {
                Serial.printlnf("Read header : %s: %s", key, value);
                if( !strcmp(key, "Content-Length") ) {
                    sscanf(value, "%zu", &recvContentLength);
                    pDataIn->setDataLen(recvContentLength);
                } else if( !strcmp(key, "Transfer-Encoding") ) {
                    if( !strcmp(value, "Chunked") || !strcmp(value, "chunked") ) {
                        recvChunked = true;
                        pDataIn->setIsChunked(true);
                    }
                } else if( !strcmp(key, "Content-Type") ) {
                    pDataIn->setDataType(value);
                } else if ( !strcmp(key, "Location") ) {
                    if (m_location) {
                        swFree(m_location);
                    }
                    m_location = (char *)swMalloc(strlen(value)+1);
                    if (m_location) {
                        strcpy(m_location,value);
                        url = m_location;
                        Serial.printlnf("Following redirect[%d] to [%s]", maxRedirect, url);
                        m_client->stop();
                        takeRedirect = true;
                        break;   // exit the while(true) header to follow the redirect
                    } else {
                        Serial.printlnf("Could not allocate memory for %s", key);
                    }
                }

                memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2) + 1); //Be sure to move NULL-terminating char as well
                trfLen -= (crlfPos + 2);
            } else {
                Serial.println("Could not parse header");
                PRTCL_ERR();
            }

        } // while(true) // get headers
        if (!takeRedirect)
            break;
    } // while (maxRedirect)

    //Receive data
    Serial.println("Receiving data");
    while(true) {
        size_t readLen = 0;

        if( recvChunked ) {
            //Read chunk header
            bool foundCrlf;
            do {
                foundCrlf = false;
                crlfPos=0;
                buf[trfLen]=0;
                if(trfLen >= 2) {
                    for(; crlfPos < trfLen - 2; crlfPos++) {
                        if( buf[crlfPos] == '\r' && buf[crlfPos + 1] == '\n' ) {
                            foundCrlf = true;
                            break;
                        }
                    }
                }
                if(!foundCrlf) { //Try to read more
                    if( trfLen < CHUNK_SIZE ) {
                        size_t newTrfLen = 0;
                        ret = recv(buf + trfLen, 0, CHUNK_SIZE - trfLen - 1, &newTrfLen);
                        trfLen += newTrfLen;
                        CHECK_CONN_ERR(ret);
                        continue;
                    } else {
                        PRTCL_ERR();
                    }
                }
            } while(!foundCrlf);
            buf[crlfPos] = '\0';
            int n = sscanf(buf, "%zu", &readLen);
            if(n!=1) {
                Serial.println("Could not read chunk length");
                PRTCL_ERR();
            }

            memmove(buf, &buf[crlfPos+2], trfLen - (crlfPos + 2)); //Not need to move NULL-terminating char any more
            trfLen -= (crlfPos + 2);

            if( readLen == 0 ) {
                //Last chunk
                break;
            }
        } else {
            readLen = recvContentLength;
        }

        Serial.printlnf("Retrieving %d bytes", readLen);

        do {
            Serial.printlnf("write %d,%d: %s", trfLen, readLen, buf);
            pDataIn->write(buf, MIN(trfLen, readLen));
            if( trfLen > readLen ) {
                memmove(buf, &buf[readLen], trfLen - readLen);
                trfLen -= readLen;
                readLen = 0;
            } else {
                readLen -= trfLen;
            }

            if(readLen) {
                ret = recv(buf, 1, CHUNK_SIZE - trfLen - 1, &trfLen);
                CHECK_CONN_ERR(ret);
                Serial.printlnf("recv'd next chunk ret: %d", ret);
            }
        } while(readLen);

        if( recvChunked ) {
            if(trfLen < 2) {
                size_t newTrfLen;
                //Read missing chars to find end of chunk
                Serial.println("read chunk");
                ret = recv(buf + trfLen, 2 - trfLen, CHUNK_SIZE - trfLen - 1, &newTrfLen);
                CHECK_CONN_ERR(ret);
                trfLen += newTrfLen;
                Serial.printlnf("recv'd next chunk ret: %d", ret);
            }
            if( (buf[0] != '\r') || (buf[1] != '\n') ) {
                Serial.println("Format error");
                PRTCL_ERR();
            }
            memmove(buf, &buf[2], trfLen - 2);
            trfLen -= 2;
        } else {
            break;
        }
    }
    m_client->stop();
    Serial.println("Completed HTTP transaction");
    return HTTP_OK;
}

HTTPResult HTTPClient::recv(char* buf, size_t minLen, size_t maxLen, size_t* pReadLen) //0 on success, err code on failure
{
    Serial.printlnf("Trying to read between %d and %d bytes", minLen, maxLen);
    size_t readLen = 0;

    if (!m_client->connected()) {
        Serial.println("Connection was closed by server");
        return HTTP_CLOSED; //Connection was closed by server
    }

    int ret;
    int start_time = System.millis();
    while(true)
    {
        if(m_client->available()) break;
        if((System.millis() - start_time) > m_timeout) return HTTP_TIMEOUT;
    }
    while (readLen < maxLen) {
        if (readLen < minLen) {
            Serial.printlnf("Trying to read at most %4d bytes [not Blocking, %d] %d,%d", minLen - readLen,
                m_timeout, minLen, readLen);
            ret = m_client->read((byte*)(buf + readLen), minLen - readLen);
        } else {
            Serial.printlnf("Trying to read at most %4d bytes [Not blocking, %d] %d,%d", maxLen - readLen,
                0, maxLen, readLen);
            ret = m_client->read((byte*)(buf + readLen), maxLen - readLen);
        }

        if (ret > 0) {
            readLen += ret;
        } else if ( ret == 0 ) {
            break;
        } else {
            if (!m_client->connected()) {
                Serial.printlnf("Connection error (recv returned %d)", ret);
                *pReadLen = readLen;
                return HTTP_CONN;
            } else {
                break;
            }
        }
        if (!m_client->connected()) {
            break;
        }
    }
    Serial.printlnf("Read %d bytes", readLen);
    buf[readLen] = '\0';    // DS makes it easier to see what's new.
    *pReadLen = readLen;
    return HTTP_OK;
}

HTTPResult HTTPClient::send(const char* buf, size_t len) //0 on success, err code on failure
{
    if(len == 0) {
        len = strlen(buf);
    }
    Serial.printlnf("send(\r\n%s,%d)", buf, len);
    size_t writtenLen = 0;

    if(!m_client->connected()) {
        Serial.println("Connection was closed by server");
        return HTTP_CLOSED; //Connection was closed by server
    }

    int ret = m_client->print(buf);
    m_client->flush();
    if(ret > 0) {
        writtenLen += ret;
    } else if( ret == 0 ) {
        Serial.println("Connection was closed by server");
        return HTTP_CLOSED; //Connection was closed by server
    } else {
        Serial.printlnf("Connection error (send returned %d)", ret);
        return HTTP_CONN;
    }
    Serial.printlnf("Written %d bytes", writtenLen);
    return HTTP_OK;
}

HTTPResult HTTPClient::parseURL(const char* url, char* scheme, size_t maxSchemeLen, char* host, size_t maxHostLen, uint16_t* port, char* path, size_t maxPathLen) //Parse URL
{
    char* schemePtr = (char*) url;
    char* hostPtr = (char*) strstr(url, "://");
    if (hostPtr == NULL) {
        Serial.println("Could not find host");
        return HTTP_PARSE; //URL is invalid
    }

    if ( (uint16_t)maxSchemeLen < hostPtr - schemePtr + 1 ) { //including NULL-terminating char
        Serial.printlnf("Scheme str is too small (%d >= %d)", maxSchemeLen, hostPtr - schemePtr + 1);
        return HTTP_PARSE;
    }
    memcpy(scheme, schemePtr, hostPtr - schemePtr);
    scheme[hostPtr - schemePtr] = '\0';

    hostPtr+=3;

    size_t hostLen = 0;

    char* portPtr = strchr(hostPtr, ':');
    if( portPtr != NULL ) {
        hostLen = portPtr - hostPtr;
        portPtr++;
        if( sscanf(portPtr, "%hu", port) != 1) {
            Serial.println("Could not find port");
            return HTTP_PARSE;
        }
    } else {
        *port=0;
    }
    Serial.printlnf("  hostPtr: %s", hostPtr);
    Serial.printlnf("  hostLen: %d", hostLen);
    char* pathPtr = strchr(hostPtr, '/');
    if( hostLen == 0 ) {
        hostLen = pathPtr - hostPtr;
    }

    if( maxHostLen < hostLen + 1 ) { //including NULL-terminating char
        Serial.printlnf("Host str is too small (%d >= %d)", maxHostLen, hostLen + 1);
        return HTTP_PARSE;
    }
    memcpy(host, hostPtr, hostLen);
    host[hostLen] = '\0';

    size_t pathLen;
    char* fragmentPtr = strchr(hostPtr, '#');
    if(fragmentPtr != NULL) {
        pathLen = fragmentPtr - pathPtr;
    } else {
        pathLen = strlen(pathPtr);
    }

    if( maxPathLen < pathLen + 1 ) { //including NULL-terminating char
        Serial.printlnf("Path str is too small (%d >= %d)", maxPathLen, pathLen + 1);
        return HTTP_PARSE;
    }
    memcpy(path, pathPtr, pathLen);
    path[pathLen] = '\0';

    return HTTP_OK;
}

void HTTPClient::createauth (const char *user, const char *pwd, char *buf, int len)
{
    char tmp[80];

    snprintf(tmp, sizeof(tmp), "%s:%s", user, pwd);
    base64enc(tmp, strlen(tmp), &buf[strlen(buf)], len - strlen(buf));
}

// Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
int HTTPClient::base64enc(const char *input, unsigned int length, char *output, int len)
{
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int c, c1, c2, c3;

    if ((uint16_t)len < ((((length-1)/3)+1)<<2)) return -1;
    for(unsigned int i = 0, j = 0; i<length; i+=3,j+=4) {
        c1 = ((((unsigned char)*((unsigned char *)&input[i]))));
        c2 = (length>i+1)?((((unsigned char)*((unsigned char *)&input[i+1])))):0;
        c3 = (length>i+2)?((((unsigned char)*((unsigned char *)&input[i+2])))):0;

        c = ((c1 & 0xFC) >> 2);
        output[j+0] = base64[c];
        c = ((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4);
        output[j+1] = base64[c];
        c = ((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6);
        output[j+2] = (length>i+1)?base64[c]:'=';
        c = (c3 & 0x3F);
        output[j+3] = (length>i+2)?base64[c]:'=';
    }
    output[(((length-1)/3)+1)<<2] = '\0';
    return 0;
}
