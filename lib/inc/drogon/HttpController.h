/**
 *
 *  HttpController.h
 *  An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#pragma once

#include <drogon/DrObject.h>
#include <drogon/utils/HttpConstraint.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpAppFrameworkManager.h>
#include <drogon/HttpOperation.h>
#include <iostream>
#include <string>
#include <trantor/utils/Logger.h>
#include <vector>

/// For more details on the class, see the wiki site (the 'HttpController'
/// section)

#define METHOD_LIST_BEGIN                                      \
    static void initPathRouting(drogon::HttpAppFramework *app) \
    {
#define METHOD_ADD(method, pattern, ...) \
    registerMethod(app, &method, pattern, {__VA_ARGS__}, true, #method)
#define ADD_METHOD_TO(method, path_pattern, ...) \
    registerMethod(app, &method, path_pattern, {__VA_ARGS__}, false, #method)
#define ADD_METHOD_VIA_REGEX(method, regex, ...) \
    registerMethodViaRegex(app, &method, regex, {__VA_ARGS__}, #method)
#define METHOD_LIST_END \
    return;             \
    }

namespace drogon
{
/**
 * @brief The base class for HTTP controllers.
 *
 */
class HttpControllerBase
{
  protected:
    HttpOperation &action;

    HttpControllerBase() : action(HttpOperation::defaultOperation)
    {
    }
};

/**
 * @brief The reflection base class template for HTTP controllers
 *
 * @tparam T the type of the implementation class
 * @tparam AutoCreation The flag for automatically creating, user can set this
 * flag to false for classes that have nondefault constructors.
 */
template <typename T, bool AutoCreation = true>
class HttpController : public DrObject<T>, public HttpControllerBase
{
  public:
    static const bool isAutoCreation = AutoCreation;

  protected:
    template <typename FUNCTION>
    static void registerMethod(
        HttpAppFramework *app,
        FUNCTION &&function,
        const std::string &pattern,
        const std::vector<internal::HttpConstraint> &filtersAndMethods =
            std::vector<internal::HttpConstraint>{},
        bool classNameInPath = true,
        const std::string &handlerName = "")
    {
        if (classNameInPath)
        {
            std::string path = "/";
            path.append(HttpController<T>::classTypeName());
            LOG_TRACE << "classname:" << HttpController<T>::classTypeName();

            // transform(path.begin(), path.end(), path.begin(), tolower);
            std::string::size_type pos;
            while ((pos = path.find("::")) != std::string::npos)
            {
                path.replace(pos, 2, "/");
            }
            if (pattern.empty() || pattern[0] == '/')
                app->registerHandler(path + pattern,
                                     std::forward<FUNCTION>(function),
                                     filtersAndMethods,
                                     handlerName);
            else
                app->registerHandler(path + "/" + pattern,
                                     std::forward<FUNCTION>(function),
                                     filtersAndMethods,
                                     handlerName);
        }
        else
        {
            std::string path = pattern;
            if (path.empty() || path[0] != '/')
            {
                path = "/" + path;
            }
            app->registerHandler(path,
                                 std::forward<FUNCTION>(function),
                                 filtersAndMethods,
                                 handlerName);
        }
    }

    template <typename FUNCTION>
    static void registerMethodViaRegex(
        HttpAppFramework *app,
        FUNCTION &&function,
        const std::string &regExp,
        const std::vector<internal::HttpConstraint> &filtersAndMethods =
            std::vector<internal::HttpConstraint>{},
        const std::string &handlerName = "")
    {
        app->registerHandlerViaRegex(regExp,
                                     std::forward<FUNCTION>(function),
                                     filtersAndMethods,
                                     handlerName);
    }

  private:
    class methodRegistrator
    {
      public:
        methodRegistrator()
        {
            // FIXME: change this
            // LOG_ERROR << "Not Implemented";
            if (AutoCreation)
            {
                // T::initPathRouting();
                HttpAppFrameworkManager::instance().pushAutoCreationFunction(
                    [](drogon::HttpAppFramework *app) {
                        T::initPathRouting(app);
                    });
            }
        }
    };
    // use static value to register controller method in framework before
    // main();
    static methodRegistrator registrator_;
    virtual void *touch()
    {
        return &registrator_;
    }
};
template <typename T, bool AutoCreation>
typename HttpController<T, AutoCreation>::methodRegistrator
    HttpController<T, AutoCreation>::registrator_;
}  // namespace drogon
