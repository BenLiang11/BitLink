#include "logger.h"
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/expressions/formatters/if.hpp>
#include <boost/log/expressions/formatter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/core.hpp>

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

void init_logging() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    logging::add_common_attributes();

    logging::add_file_log(
        keywords::file_name = "logs/server.log",        
        keywords::open_mode = std::ios_base::app,
        keywords::format = (
            expr::stream
                << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
                << "] [Thread " << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
                << "] [" << logging::trivial::severity
                << "] " << expr::smessage
        )
    );
    
    // Add console logging to stdout with the same format
    logging::add_console_log(
        std::cout,
        keywords::format = (
            expr::stream
                << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
                << "] [Thread " << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
                << "] [" << logging::trivial::severity
                << "] " << expr::smessage
        )
    );

    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );

    BOOST_LOG_TRIVIAL(info) << "starting up";
}