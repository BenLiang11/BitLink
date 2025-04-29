#include "logger.h"
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/expressions/formatters/if.hpp>
#include <boost/log/expressions/formatter.hpp>

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

void init_logging() {
    logging::add_common_attributes();

    // Console output
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

    // File output with rotation
    logging::add_file_log(
        keywords::file_name = "logs/server_%Y-%m-%d_%H-%M-%S.log",
        keywords::rotation_size = 10 * 1024 * 1024, // 10MB 넘어가면 새로
        keywords::time_based_rotation = logging::sinks::file::rotation_at_time_point(0, 0, 0), // 자정마다
        keywords::format = (
            expr::stream
                << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
                << "] [Thread " << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
                << "] [" << logging::trivial::severity
                << "] " << expr::smessage
        )
    );
}
