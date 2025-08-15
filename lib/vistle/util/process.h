#ifndef VISTLE_UTIL_PROCESS_H
#define VISTLE_UTIL_PROCESS_H

#include <boost/version.hpp>

#if BOOST_VERSION >= 108000
#define HAVE_BOOST_PROCESS_V2
#endif

#if BOOST_VERSION >= 108800
#define BOOST_PROCESS_V1_DEPRECATED
#endif

#ifdef BOOST_PROCESS_V1_DEPRECATED
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/system.hpp>
#include <boost/process/v1/search_path.hpp>
#include <boost/process/v1/args.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/spawn.hpp>
#include <boost/process/v1/extend.hpp>
#include <boost/process/v1/env.hpp>

namespace process = boost::process::v1;
#else
#include <boost/process.hpp>
#include <boost/process/extend.hpp>

namespace process = boost::process;
#endif

#endif
