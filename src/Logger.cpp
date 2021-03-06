#include "Wrapper.h"

#include <boost/core/null_deleter.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/log/keywords/format.hpp>
#include <boost/log/keywords/severity.hpp>
namespace keywords = boost::log::keywords;

#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
namespace expr = boost::log::expressions;

#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
namespace sinks = boost::log::sinks;

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>

severity_level g_logging_level = error;

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, SEVERITY_ATTR, severity_level);
BOOST_LOG_ATTRIBUTE_KEYWORD(process_name, PROCESS_NAME_ATTR, std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(isolate, ISOLATE_ATTR, const v8::Isolate *);
BOOST_LOG_ATTRIBUTE_KEYWORD(context, CONTEXT_ATTR, const v8::Context *);
BOOST_LOG_ATTRIBUTE_KEYWORD(script_name, SCRIPT_NAME_ATTR, std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(script_line_no, SCRIPT_LINE_NO_ATTR, int);
BOOST_LOG_ATTRIBUTE_KEYWORD(script_column_no, SCRIPT_COLUMN_NO_ATTR, int);

typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;

template <typename CharT, typename TraitsT>
inline std::basic_istream<CharT, TraitsT> &operator>>(
    std::basic_istream<CharT, TraitsT> &stream, severity_level &level)
{
    std::string s;

    stream >> s;

    boost::to_upper(s);

    if (s.compare("TRACE") == 0)
        level = trace;
    else if (s.compare("DEBUG") == 0)
        level = debug;
    else if (s.compare("INFO") == 0)
        level = info;
    else if (s.compare("WARNING") == 0)
        level = warning;
    else if (s.compare("ERROR") == 0)
        level = error;
    else if (s.compare("FATAL") == 0)
        level = fatal;
    else
        throw std::invalid_argument(s);

    return stream;
}

template <typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT> &operator<<(
    std::basic_ostream<CharT, TraitsT> &stream, severity_level level)
{
    switch (level)
    {
    case trace:
        stream << "TRACE";
        break;
    case debug:
        stream << "DEBUG";
        break;
    case info:
        stream << "INFO";
        break;
    case warning:
        stream << "WARNING";
        break;
    case error:
        stream << "ERROR";
        break;
    case fatal:
        stream << "FATAL";
        break;
    }

    return stream;
}

void initialize_logging()
{
    const char *pyv8_log = getenv("PYV8_LOG");

    if (pyv8_log)
    {
        std::istringstream(pyv8_log) >> g_logging_level;
    }

    logging::register_simple_formatter_factory<severity_level, char>(SEVERITY_ATTR);
    logging::register_simple_filter_factory<severity_level>(SEVERITY_ATTR);

    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

    sink->locked_backend()->add_stream(
        boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));

    sink->locked_backend()->auto_flush(true);

    logging::add_common_attributes();

    logging::core::get()->add_global_attribute(PROCESS_NAME_ATTR, attrs::current_process_name());
    logging::core::get()->add_global_attribute(SCOPE_ATTR, attrs::named_scope());

    sink->set_formatter(
        expr::stream << expr::format_date_time<boost::posix_time::ptime>(TIMESTAMP_ATTR, "%Y-%m-%d %H:%M:%S") << " [" << std::dec << expr::attr<logging::process_id>(PROCESS_ID_ATTR) << ":" << expr::attr<logging::thread_id>(THREAD_ID_ATTR) << "]" << expr::if_(g_logging_level <= debug && expr::has_attr(isolate))[expr::stream << " <" << isolate << ":" << context << ">"]
                     << expr::if_(expr::has_attr(SCRIPT_NAME_ATTR))
                            [expr::stream << " {" << script_name << "@" << script_line_no << ":" << script_column_no << "}"]
                     << " " << severity << " " << expr::message);

    sink->set_filter(severity >= boost::ref(g_logging_level));

    logging::core::get()->add_sink(sink);
}
