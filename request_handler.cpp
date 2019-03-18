//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time.hpp>
#include "httputils.h"
#include <sstream>
#include "range.h"

namespace http {
namespace server3 {

request_handler::request_handler(const std::string& doc_root) : doc_root_(doc_root) { }

void request_handler::handle_request(const request& req, reply& rep) {
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path)) {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  std::cout << "Request path: " << request_path << std::endl;
  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..",0) != std::string::npos) {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/') {
    request_path += "index.html";
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos) {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  std::string full_path = doc_root_ + request_path;
  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
  if (!is) {
    rep = reply::stock_reply(reply::not_found);
    return;
  }

  std::ifstream in(full_path.c_str(), std::ifstream::ate | std::ifstream::binary);
  long long int length = in.tellg();
  in.close();

  std::cout << "File size: " << length << std::endl;

  std::string filename = request_path;

  struct stat info;
  stat("/etc", &info);
  long long int modification_ms = info.st_mtimespec.tv_sec * 1000 + info.st_mtimespec.tv_nsec / 1000000;
  std::cout << "File last modified time: " << modification_ms << std::endl;
  long long int ms = std::chrono::duration_cast<std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch()).count();
  std::cout << "Current time millis: " << ms << std::endl;
  std::string content_type =  mime_types::extension_to_type(extension);
  std::cout << "File content type: " << content_type << std::endl;

  std::string if_none_match_header = getHeader(req, "If-None-Match");

  if(!if_none_match_header.empty() && httputils::matches(if_none_match_header, filename)){
      rep.status = reply::not_modified;
      rep.headers.resize(1);
      rep.headers[0].name = "ETag";
      rep.headers[0].value = filename;
      std::cout << "Status 'Not modified' because 'If-None-Match' condition" << std::endl;
      return;
  }

    long long int if_modified_since = getDateHeader(req, "If-Modified-Since");

    if(if_none_match_header.empty() && if_modified_since != -1 && if_modified_since + 1000 > modification_ms){
        rep.status = reply::not_modified;
        rep.headers.resize(1);
        rep.headers[0].name = "ETag";
        rep.headers[0].value = filename;
        std::cout << "Status 'Not modified' because 'If-Modified-Since' condition" << std::endl;
        return;
    }

    std::string if_match = getHeader(req, "If-Match");
    if(!if_match.empty() && !httputils::matches(if_match, filename)){
        rep = reply::stock_reply(reply::precondition_failed);
        std::cout << "Status 'Precondition failed' because 'If-Match' condition" << std::endl;
        return;
    }

    long long int if_unmodified_since = getDateHeader(req, "If-Unmodified-Since");
    if(if_unmodified_since != -1 && if_unmodified_since + 1000 <= modification_ms){
        rep = reply::stock_reply(reply::precondition_failed);
        std::cout << "Status 'Precondition failed' because 'If-Unmodified-Since' condition" << std::endl;
        return;
    }

    range full(0, length - 1, length);

    std::vector<range> ranges;

    std::string range_value = getHeader(req, "Range");
    if(!range_value.empty()){
        if (!std::regex_match (range_value, std::regex("^bytes=\\d*-\\d*(,\\d*-\\d*)*$") )){
            rep.status = reply::requested_range_not_satisfiable;
            rep.headers.resize(1);
            rep.headers[0].name = "Content-Range";
            rep.headers[0].value = "bytes */" + std::to_string(length);
            return;
        }

        std::string if_range = getHeader(req, "If-Range");
        if(!if_range.empty() && if_range != filename){
            long long int if_range_time = getDateHeader(req, "If-Range");
            if(if_range_time != -1) {
                std::cout << "Returning full range because 'If-Range' condition" << std::endl;
                ranges.push_back(full);
            }
        }

        if(ranges.empty()){
            std::vector<std::string> parts;
            httputils::split(range_value.substr(6), parts, ",");

            for(const std::string& part: parts){
                long start = range::sublong(part, 0, part.find("-",0));
                long end = range::sublong(part, part.find("-",0) + 1, part.length());

                std::cout << "ORIGINAL START: " << start << std::endl;
                std::cout << "ORIGINAL END: " << end << std::endl;

                if (start == -1) {
                    start = length - end;
                    if(length - start < range::DEFAULT_BUFFER_SIZE) {
                        end = length - 1;
                    } else
                        end = start + range::DEFAULT_BUFFER_SIZE;
                } else if (end == -1 || end > length - 1) {
                    if(length - start < range::DEFAULT_BUFFER_SIZE) {
                        end = length - 1;
                    }else
                        end = start + range::DEFAULT_BUFFER_SIZE;
                }

                if(start > end){
                    rep.status = reply::requested_range_not_satisfiable;
                    rep.headers.resize(1);
                    rep.headers[0].name = "Content-Range";
                    rep.headers[0].value = "bytes */" + std::to_string(length);
                    return;
                }
                ranges.emplace_back(range(start, end, length));
            }
        }
    }

    std::string disposition = "inline";

    if(content_type.empty())
        content_type = "application/octet-stream";
    else if(content_type.rfind("image", 0) != 0){
        std::string accept = getHeader(req, "Accept");
        disposition = !accept.empty() && httputils::accepts(accept, content_type) ? "inline" : "attachment";
    }

    std::cout << "Content-Type: " << content_type << std::endl;
    rep.headers.resize(8);
    rep.headers[0].name = "Content-Type";
    rep.headers[0].value = content_type;

    rep.headers[1].name = "Content-Disposition";
    rep.headers[1].value = disposition + ";filename=\"" + filename + "\"";

    std::cout << "Content-Disposition: " << disposition << std::endl;

    rep.headers[2].name = "Accept-Ranges";
    rep.headers[2].value = "bytes";

    rep.headers[3].name = "ETag";
    rep.headers[3].value = filename;

    rep.headers[4].name = "Last-Modified";
    rep.headers[4].value = std::to_string(modification_ms);

    long long int ms1 = std::chrono::duration_cast<std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch()).count();

    rep.headers[5].name = "Expires";
    rep.headers[5].value = std::to_string(ms1 + 604800000L);

    if(ranges.empty() || &ranges.at(0) == &full){
        std::cout << "Returning full file" << std::endl;
        rep.status = reply::ok;
        rep.headers[6].name = "Content-Range";
        rep.headers[6].value = "bytes " + std::to_string(full.start) + "-" + std::to_string(full.end) + "/" + std::to_string(full.total);
        rep.headers[7].name = "Content-Length";
        rep.headers[7].value = std::to_string(full.length);
        range::copy(is, rep, length, full.start, full.length);
    } else if(ranges.size() == 1){
        range r = ranges.at(0);
        std::cout << "Return 1 part of file : from " << r.start << " to " << r.end << std::endl;
        rep.headers[6].name = "Content-Range";
        rep.headers[6].value = "bytes " + std::to_string(r.start) + "-" + std::to_string(r.end) + "/" + std::to_string(r.total);
        rep.headers[7].name = "Content-Length";
        rep.headers[7].value = std::to_string(r.length);
        rep.status = reply::partial_content;
        range::copy(is, rep, length, r.start, r.length);
    }else{
        rep.headers[0].name = "Content-Type";
        rep.headers[0].value = "multipart/byteranges; boundary=MULTIPART_BYTERANGES";
        rep.status = reply::partial_content;
        for (range r : ranges) {
            std::cout << "Return multi part of file : from " << r.start << " to " << r.end;
            rep.content.append("\n");
            rep.content.append("--MULTIPART_BYTERANGES\n");
            rep.content.append("Content-Type: " + content_type + "\n");
            rep.content.append("Content-Range: bytes " + std::to_string(r.start) + "-" + std::to_string(r.end) + "/" + std::to_string(r.total));
            range::copy(is, rep, length, r.start, r.length);
        }
    }
}

std::string request_handler::getHeader(const request& req, const std::string& name){
    for(http::server3::header header1: req.headers){
        if(header1.name == name){
            return header1.value;
        }
    }
    return "";
}

long long int request_handler::getDateHeader(const request& req, const std::string& name){
    for(http::server3::header header1: req.headers){
        if(header1.name == name){
            boost::posix_time::ptime pt;{
                std::istringstream iss(header1.value);
                auto* f = new boost::posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S %Z *!");
                std::locale loc(std::locale(""), f);
                iss.imbue(loc);
                iss >> pt;
            }
            return (pt - boost::posix_time::ptime{{1970,1,1},{}}).total_milliseconds();
        }
    }
    return -1;
}

bool request_handler::url_decode(const std::string& in, std::string& out) {
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '%') {
      if (i + 3 <= in.size()) {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value) {
          out += static_cast<char>(value);
          i += 2;
        } else {
          return false;
        }
      } else {
        return false;
      }
    } else if (in[i] == '+') {
      out += ' ';
    } else {
      out += in[i];
    }
  }
  return true;
}

} // namespace server3
} // namespace http
