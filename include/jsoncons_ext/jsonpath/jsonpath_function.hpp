// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONPATH_JSONPATH_FUNCTION_HPP
#define JSONCONS_JSONPATH_JSONPATH_FUNCTION_HPP

#include <string> // std::basic_string
#include <vector> // std::vector
#include <unordered_map> // std::unordered_map
#include <limits> // std::numeric_limits
#include <utility> // std::move
#include <jsoncons/jsoncons_utilities.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>

namespace jsoncons { namespace jsonpath {

JSONCONS_DEFINE_LITERAL(avg_literal,"avg")
JSONCONS_DEFINE_LITERAL(max_literal,"max")
JSONCONS_DEFINE_LITERAL(min_literal,"min")
JSONCONS_DEFINE_LITERAL(sum_literal,"sum")
JSONCONS_DEFINE_LITERAL(prod_literal,"prod")
JSONCONS_DEFINE_LITERAL(count_literal,"count")
JSONCONS_DEFINE_LITERAL(tokenize_literal,"tokenize")

template <class JsonPointer>
class node_set 
{
public:
    typedef JsonPointer pointer;
private:
    std::vector<pointer> nodes_;
public:
    node_set(const std::vector<pointer>& nodes)
        : nodes_(nodes)
    {
    }
    node_set(std::vector<pointer>&& nodes)
        : nodes_(std::move(nodes))
    {
    }

    const std::vector<pointer>& nodes() const 
    {
        return nodes_;
    }
};

template <class Json, class JsonPointer>
class function_table
{
public:
    typedef typename Json::char_type char_type;
    typedef typename Json::char_traits_type char_traits_type;
    typedef std::basic_string<char_type,char_traits_type> string_type;
    typedef typename Json::string_view_type string_view_type;
    typedef JsonPointer pointer ;
    typedef std::function<Json(const std::vector<node_set<pointer>>&, std::error_code&)> function_type;
    typedef std::unordered_map<string_type,function_type> function_dictionary;
private:
    const function_dictionary functions_ =
    {
        {
            max_literal<char_type>(),[](const std::vector<node_set<pointer>>& args, std::error_code& ec)
                {
                   if (args.size() != 1)
                   {
                       ec = jsonpath_errc::invalid_function_argument;
                       return Json(); 
                   }
                    const auto& arg = args[0];
                    double v = std::numeric_limits<double>::lowest();
                    for (auto& node : arg.nodes())
                    {
                        double x = node->template as<double>();
                        if (x > v)
                        {
                            v = x;
                        }
                    }
                    return Json(v);
                }
        },
        {
            min_literal<char_type>(),[](const std::vector<node_set<pointer>>& args, std::error_code& ec) 
                {
                    if (args.size() != 1)
                    {
                        ec = jsonpath_errc::invalid_function_argument;
                        return Json();
                    }
                    const auto& arg = args[0];
                    double v = (std::numeric_limits<double>::max)(); 
                    for (const auto& node : arg.nodes())
                    {
                        double x = node->template as<double>();
                        if (x < v)
                        {
                            v = x;
                        }
                    }
                    return Json(v);
                }
        },
        {
            avg_literal<char_type>(),[](const std::vector<node_set<pointer>>& args, std::error_code& ec)
                {
                    if (args.size() != 1)
                    {
                        ec = jsonpath_errc::invalid_function_argument;
                        return Json();
                    }
                    const auto& arg = args[0];
                    double v = 0.0;
                    for (const auto& node : arg.nodes())
                    {
                        v += node->template as<double>();
                    }
                    return arg.nodes().size() > 0 ? Json(v/arg.nodes().size()) : Json::null();
                }
        },
        {
            sum_literal<char_type>(),[](const std::vector<node_set<pointer>>& args, std::error_code& ec)
                {
                    if (args.size() != 1)
                    {
                        ec = jsonpath_errc::invalid_function_argument;
                        return Json();
                    }
                    const auto& arg = args[0];
                    double v = 0.0;
                    for (const auto& node : arg.nodes())
                    {
                        v += node->template as<double>();
                    }
                    return Json(v);
                }
        },
        {
            count_literal<char_type>(),[](const std::vector<node_set<pointer>>& args, std::error_code& ec)
                {
                    if (args.size() != 1)
                    {
                        ec = jsonpath_errc::invalid_function_argument;
                        return Json();
                    }
                    const auto& arg = args[0];
                    size_t count = 0;
                    while (count < arg.nodes().size())
                    {
                        ++count;
                    }
                    return Json(count);
                }
        },
        {
            prod_literal<char_type>(),[](const std::vector<node_set<pointer>>& args, std::error_code& ec)
                {
                    if (args.size() != 1)
                    {
                        ec = jsonpath_errc::invalid_function_argument;
                        return Json();
                    }
                    const auto& arg = args[0];
                    double v = 0.0;
                    for (const auto& node : arg.nodes())
                    {
                        double x = node->template as<double>();
                        v == 0.0 && x != 0.0
                        ? (v = x)
                        : (v *= x);

                    }
                    return Json(v);
                }
        }
#if !defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 9)
// GCC 4.8 has broken regex support: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53631
        ,{
            tokenize_literal<char_type>(),[](const std::vector<node_set<pointer>>& args, std::error_code& ec)
                {
                    if (args.size() != 2)
                    {
                        ec = jsonpath_errc::invalid_function_argument;
                        return Json();
                    }
                    string_type arg1 = args[0].nodes()[0]->as_string();
                    string_type arg2 = args[1].nodes()[0]->as_string();

                    std::regex::flag_type flags = std::regex_constants::ECMAScript; 
                    std::basic_regex<char_type> pieces_regex(arg2, flags);

                    std::regex_token_iterator<typename string_type::const_iterator> rit ( arg1.begin(), arg1.end(), pieces_regex, -1);
                    std::regex_token_iterator<typename string_type::const_iterator> rend;

                    Json j = Json::array();
                    while (rit!=rend) 
                    {
                        j.push_back(rit->str());
                        ++rit;
                    }
                    return j;
                }
#endif
        }
    };
public:
    function_type get(const string_type& name, std::error_code& ec) const
    {
        auto it = functions_.find(name);
        if (it == functions_.end())
        {
            ec = jsonpath_errc::function_name_not_found;
            return nullptr;
        }
        return it->second;
    }
};

}}

#endif