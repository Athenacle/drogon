/**
 *
 *  @file ConfigLoader.cc
 *  @author An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#include "ConfigLoader.h"
#include "HttpAppFrameworkImpl.h"
#include <drogon/config.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <trantor/utils/Logger.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

using namespace drogon;
static bool bytesSize(std::string &sizeStr, size_t &size)
{
    if (sizeStr.empty())
    {
        size = -1;
        return true;
    }
    else
    {
        size = 1;
        switch (sizeStr[sizeStr.length() - 1])
        {
            case 'k':
            case 'K':
                size = 1024;
                sizeStr.resize(sizeStr.length() - 1);
                break;
            case 'M':
            case 'm':
                size = (1024 * 1024);
                sizeStr.resize(sizeStr.length() - 1);
                break;
            case 'g':
            case 'G':
                size = (1024 * 1024 * 1024);
                sizeStr.resize(sizeStr.length() - 1);
                break;
#if ((ULONG_MAX) != (UINT_MAX))
            // 64bit system
            case 't':
            case 'T':
                size = (1024L * 1024L * 1024L * 1024L);
                sizeStr.resize(sizeStr.length() - 1);
                break;
#endif
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '7':
            case '8':
            case '9':
                break;
            default:
                return false;
                break;
        }
        std::istringstream iss(sizeStr);
        size_t tmpSize;
        iss >> tmpSize;
        if (iss.fail())
        {
            return false;
        }
        if ((size_t(-1) / tmpSize) >= size)
            size *= tmpSize;
        else
        {
            size = -1;
        }
        return true;
    }
}
ConfigLoader::ConfigLoader(HttpAppFrameworkImpl *app,
                           const std::string &configFile)
    : app_(app)
{
#ifdef _WIN32
    if (_access(configFile.c_str(), 0) != 0)
#else
    if (access(configFile.c_str(), 0) != 0)
#endif
    {
        std::cerr << "Config file " << configFile << " not found!" << std::endl;
        exit(1);
    }
#ifdef _WIN32
    if (_access(configFile.c_str(), 04) != 0)
#else
    if (access(configFile.c_str(), R_OK) != 0)
#endif
    {
        std::cerr << "No permission to read config file " << configFile
                  << std::endl;
        exit(1);
    }
    configFile_ = configFile;
    std::ifstream infile(configFile.c_str(), std::ifstream::in);
    if (infile)
    {
        try
        {
            infile >> configJsonRoot_;
        }
        catch (const std::exception &exception)
        {
            std::cerr << "Configuration file format error! in " << configFile
                      << ":" << std::endl;
            std::cerr << exception.what() << std::endl;
            exit(1);
        }
    }
}
ConfigLoader::ConfigLoader(HttpAppFrameworkImpl *app, const Json::Value &data)
    : app_(app), configJsonRoot_(data)
{
}
ConfigLoader::ConfigLoader(HttpAppFrameworkImpl *app, Json::Value &&data)
    : app_(app), configJsonRoot_(std::move(data))
{
}
ConfigLoader::~ConfigLoader()
{
}
static void loadLogSetting(HttpAppFrameworkImpl *app, const Json::Value &log)
{
    if (!log)
        return;
    auto logPath = log.get("log_path", "").asString();
    if (logPath != "")
    {
        auto baseName = log.get("logfile_base_name", "").asString();
        auto logSize = log.get("log_size_limit", 100000000).asUInt64();
        app->setLogPath(logPath, baseName, logSize);
    }
    auto logLevel = log.get("log_level", "DEBUG").asString();
    if (logLevel == "TRACE")
    {
        trantor::Logger::setLogLevel(trantor::Logger::kTrace);
    }
    else if (logLevel == "DEBUG")
    {
        trantor::Logger::setLogLevel(trantor::Logger::kDebug);
    }
    else if (logLevel == "INFO")
    {
        trantor::Logger::setLogLevel(trantor::Logger::kInfo);
    }
    else if (logLevel == "WARN")
    {
        trantor::Logger::setLogLevel(trantor::Logger::kWarn);
    }
}
static void loadControllers(HttpAppFrameworkImpl *app,
                            const Json::Value &controllers)
{
    if (!controllers)
        return;
    for (auto const &controller : controllers)
    {
        auto path = controller.get("path", "").asString();
        auto ctrlName = controller.get("controller", "").asString();
        if (path == "" || ctrlName == "")
            continue;
        std::vector<internal::HttpConstraint> constraints;
        if (!controller["http_methods"].isNull())
        {
            for (auto const &method : controller["http_methods"])
            {
                auto strMethod = method.asString();
                std::transform(strMethod.begin(),
                               strMethod.end(),
                               strMethod.begin(),
                               tolower);
                if (strMethod == "get")
                {
                    constraints.push_back(Get);
                }
                else if (strMethod == "post")
                {
                    constraints.push_back(Post);
                }
                else if (strMethod == "head")  // The branch nerver work
                {
                    constraints.push_back(Head);
                }
                else if (strMethod == "put")
                {
                    constraints.push_back(Put);
                }
                else if (strMethod == "delete")
                {
                    constraints.push_back(Delete);
                }
                else if (strMethod == "patch")
                {
                    constraints.push_back(Patch);
                }
            }
        }
        if (!controller["filters"].isNull())
        {
            for (auto const &filter : controller["filters"])
            {
                constraints.push_back(filter.asString());
            }
        }
        app->registerHttpSimpleController(path, ctrlName, constraints);
    }
}
static void loadApp(HttpAppFrameworkImpl *app, const Json::Value &json)
{
    if (!json)
        return;
    // threads number
    auto threadsNum = json.get("threads_num", 1).asUInt64();
    if (threadsNum == 0)
    {
        // set the number to the number of processors.
        threadsNum = std::thread::hardware_concurrency();
        LOG_TRACE << "The number of processors is " << threadsNum;
    }
    if (threadsNum < 1)
        threadsNum = 1;
    app->setThreadNum(threadsNum);
    // session
    auto enableSession = json.get("enable_session", false).asBool();
    auto timeout = json.get("session_timeout", 0).asUInt64();
    if (enableSession)
        app->enableSession(timeout);
    else
        app->disableSession();
    // document root
    auto documentRoot = json.get("document_root", "").asString();
    if (documentRoot != "")
    {
        app->setDocumentRoot(documentRoot);
    }
    if (!json["static_file_headers"].empty())
    {
        if (json["static_file_headers"].isArray())
        {
            std::vector<std::pair<std::string, std::string>> headers;
            for (auto &header : json["static_file_headers"])
            {
                headers.emplace_back(std::make_pair<std::string, std::string>(
                    header["name"].asString(), header["value"].asString()));
            }
            app->setStaticFileHeaders(headers);
        }
        else
        {
            std::cerr << "The static_file_headers option must be an array\n";
            exit(1);
        }
    }
    // upload path
    auto uploadPath = json.get("upload_path", "uploads").asString();
    app->setUploadPath(uploadPath);
    // file types
    auto fileTypes = json["file_types"];
    if (fileTypes.isArray() && !fileTypes.empty())
    {
        std::vector<std::string> types;
        for (auto const &fileType : fileTypes)
        {
            types.push_back(fileType.asString());
            LOG_TRACE << "file type:" << types.back();
        }
        app->setFileTypes(types);
    }
    // locations
    if (json.isMember("locations"))
    {
        auto &locations = json["locations"];
        if (!locations.isArray())
        {
            std::cerr << "The locations option must be an array\n";
            exit(1);
        }
        for (auto &location : locations)
        {
            auto uri = location.get("uri_prefix", "").asString();
            if (uri.empty())
                continue;
            auto defaultContentType =
                location.get("default_content_type", "").asString();
            auto alias = location.get("alias", "").asString();
            auto isCaseSensitive =
                location.get("is_case_sensitive", false).asBool();
            auto allAll = location.get("allow_all", true).asBool();
            auto isRecursive = location.get("is_recursive", true).asBool();
            if (!location["filters"].isNull())
            {
                if (location["filters"].isArray())
                {
                    std::vector<std::string> filters;
                    for (auto const &filter : location["filters"])
                    {
                        filters.push_back(filter.asString());
                    }
                    app->addALocation(uri,
                                      defaultContentType,
                                      alias,
                                      isCaseSensitive,
                                      allAll,
                                      isRecursive,
                                      filters);
                }
                else
                {
                    std::cerr << "the filters of location '" << uri
                              << "' should be an array" << std::endl;
                    exit(1);
                }
            }
            else
            {
                app->addALocation(uri,
                                  defaultContentType,
                                  alias,
                                  isCaseSensitive,
                                  allAll,
                                  isRecursive);
            }
        }
    }
    // max connections
    auto maxConns = json.get("max_connections", 0).asUInt64();
    if (maxConns > 0)
    {
        app->setMaxConnectionNum(maxConns);
    }
    // max connections per IP
    auto maxConnsPerIP = json.get("max_connections_per_ip", 0).asUInt64();
    if (maxConnsPerIP > 0)
    {
        app->setMaxConnectionNumPerIP(maxConnsPerIP);
    }
#ifndef _WIN32
    // dynamic views
    auto enableDynamicViews = json.get("load_dynamic_views", false).asBool();
    if (enableDynamicViews)
    {
        auto viewsPaths = json["dynamic_views_path"];
        if (viewsPaths.isArray() && viewsPaths.size() > 0)
        {
            std::vector<std::string> paths;
            for (auto const &viewsPath : viewsPaths)
            {
                paths.push_back(viewsPath.asString());
                LOG_TRACE << "views path:" << paths.back();
            }
            auto outputPath =
                json.get("dynamic_views_output_path", "").asString();
            app->enableDynamicViewsLoading(paths, outputPath);
        }
    }
#endif
    auto unicodeEscaping =
        json.get("enable_unicode_escaping_in_json", true).asBool();
    app->setUnicodeEscapingInJson(unicodeEscaping);
    auto &precision = json["float_precision_in_json"];
    if (!precision.isNull())
    {
        auto precisionLength = precision.get("precision", 0).asUInt64();
        auto precisionType =
            precision.get("precision_type", "significant").asString();
        app->setFloatPrecisionInJson(precisionLength, precisionType);
    }
    // log
    loadLogSetting(app, json["log"]);
    // run as daemon
    auto runAsDaemon = json.get("run_as_daemon", false).asBool();
    if (runAsDaemon)
    {
        app->enableRunAsDaemon();
    }
    // relaunch
    auto relaunch = json.get("relaunch_on_error", false).asBool();
    if (relaunch)
    {
        app->enableRelaunchOnError();
    }
    auto useSendfile = json.get("use_sendfile", true).asBool();
    app->enableSendfile(useSendfile);
    auto useGzip = json.get("use_gzip", true).asBool();
    app->enableGzip(useGzip);
    auto useBr = json.get("use_brotli", false).asBool();
    app->enableBrotli(useBr);
    auto staticFilesCacheTime = json.get("static_files_cache_time", 5).asInt();
    app->setStaticFilesCacheTime(staticFilesCacheTime);
    loadControllers(app, json["simple_controllers_map"]);
    // Kick off idle connections
    auto kickOffTimeout = json.get("idle_connection_timeout", 60).asUInt64();
    app->setIdleConnectionTimeout(kickOffTimeout);
    auto server = json.get("server_header_field", "").asString();
    if (!server.empty())
        app->setServerHeaderField(server);
    auto sendServerHeader = json.get("enable_server_header", true).asBool();
    app->enableServerHeader(sendServerHeader);
    auto sendDateHeader = json.get("enable_date_header", true).asBool();
    app->enableDateHeader(sendDateHeader);
    auto keepaliveReqs = json.get("keepalive_requests", 0).asUInt64();
    app->setKeepaliveRequestsNumber(keepaliveReqs);
    auto pipeliningReqs = json.get("pipelining_requests", 0).asUInt64();
    app->setPipeliningRequestsNumber(pipeliningReqs);
    auto useGzipStatic = json.get("gzip_static", true).asBool();
    app->setGzipStatic(useGzipStatic);
    auto useBrStatic = json.get("br_static", true).asBool();
    app->setBrStatic(useBrStatic);
    auto maxBodySize = json.get("client_max_body_size", "1M").asString();
    size_t size;
    if (bytesSize(maxBodySize, size))
    {
        app->setClientMaxBodySize(size);
    }
    else
    {
        std::cerr << "Error format of client_max_body_size" << std::endl;
        exit(1);
    }
    auto maxMemoryBodySize =
        json.get("client_max_memory_body_size", "64K").asString();
    if (bytesSize(maxMemoryBodySize, size))
    {
        app->setClientMaxMemoryBodySize(size);
    }
    else
    {
        std::cerr << "Error format of client_max_memory_body_size" << std::endl;
        exit(1);
    }
    auto maxWsMsgSize =
        json.get("client_max_websocket_message_size", "128K").asString();
    if (bytesSize(maxWsMsgSize, size))
    {
        app->setClientMaxWebSocketMessageSize(size);
    }
    else
    {
        std::cerr << "Error format of client_max_websocket_message_size"
                  << std::endl;
        exit(1);
    }
    app->enableReusePort(json.get("reuse_port", false).asBool());
    app->setHomePage(json.get("home_page", "index.html").asString());
    app->setImplicitPageEnable(json.get("use_implicit_page", true).asBool());
    app->setImplicitPage(json.get("implicit_page", "index.html").asString());
}
static void loadDbClients(HttpAppFrameworkImpl *app,
                          const Json::Value &dbClients)
{
    if (!dbClients)
        return;
    for (auto const &client : dbClients)
    {
        auto type = client.get("rdbms", "postgresql").asString();
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        auto host = client.get("host", "127.0.0.1").asString();
        auto port = client.get("port", 5432).asUInt();
        auto dbname = client.get("dbname", "").asString();
        if (dbname == "" && type != "sqlite3")
        {
            std::cerr << "Please configure dbname in the configuration file"
                      << std::endl;
            exit(1);
        }
        auto user = client.get("user", "postgres").asString();
        auto password = client.get("passwd", "").asString();
        if (password.empty())
        {
            password = client.get("password", "").asString();
        }
        auto connNum = client.get("connection_number", 1).asUInt();
        auto name = client.get("name", "default").asString();
        auto filename = client.get("filename", "").asString();
        auto isFast = client.get("is_fast", false).asBool();
        auto characterSet = client.get("characterSet", "").asString();
        if (characterSet.empty())
        {
            characterSet = client.get("client_encoding", "").asString();
        }
        app->createDbClient(type,
                            host,
                            (unsigned short)port,
                            dbname,
                            user,
                            password,
                            connNum,
                            filename,
                            name,
                            isFast,
                            characterSet);
    }
}
static void loadListeners(HttpAppFrameworkImpl *app,
                          const Json::Value &listeners)
{
    if (!listeners)
        return;
    LOG_TRACE << "Has " << listeners.size() << " listeners";
    for (auto const &listener : listeners)
    {
        auto addr = listener.get("address", "0.0.0.0").asString();
        auto port = (uint16_t)listener.get("port", 0).asUInt();
        auto useSSL = listener.get("https", false).asBool();
        auto cert = listener.get("cert", "").asString();
        auto key = listener.get("key", "").asString();
        auto useOldTLS = listener.get("use_old_tls", false).asBool();
        LOG_TRACE << "Add listener:" << addr << ":" << port;
        app->addListener(addr, port, useSSL, cert, key, useOldTLS);
    }
}
static void loadSSL(HttpAppFrameworkImpl *app, const Json::Value &sslFiles)
{
    if (!sslFiles)
        return;
    auto key = sslFiles.get("key", "").asString();
    auto cert = sslFiles.get("cert", "").asString();
    app->setSSLFiles(cert, key);
}
void ConfigLoader::load()
{
    // std::cout<<configJsonRoot_<<std::endl;
    loadApp(app_, configJsonRoot_["app"]);
    loadSSL(app_, configJsonRoot_["ssl"]);
    loadListeners(app_, configJsonRoot_["listeners"]);
    loadDbClients(app_, configJsonRoot_["db_clients"]);
}
