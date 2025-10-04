
#include "text_view_handler.h"
#include <sstream>
#include <unistd.h> 
#include <iostream> 
#include <fstream>
#include <boost/regex.hpp>
#include <regex>
namespace http {
namespace server {
std::unique_ptr<reply> TextViewHandler::handle_request(const request& request) {
    std::string id;
    // allow special chars in uri
    std::string uri = urlDecode(request.uri);
    if (!parse_uri(uri, id))
        return BuildResponse(reply::bad_request, "Invalid request uri\r\n");
    
    std::string content;
    if (!read_file(id, content))
        return BuildResponse(reply::not_found, "File could not be found\r\n");
    
    std::string file_extension;
    if (!parse_file_extension(id, file_extension))
        return BuildResponse(reply::internal_server_error, "File extension could not be extracted\r\n");
    if (file_extension == "md") { // Markdown files
        std::vector<header> headers;
        header content_type = {"Content-Type", "text/html"};
        headers.push_back(content_type);
        return BuildResponse(reply::ok, render_markdown(content), headers);
    } else if (file_extension == "pdf") { // PDF files
        if (read_pdf(content, id))
            return BuildResponse(reply::ok, content);
        else
            return BuildResponse(reply::internal_server_error, "Text from PDF could not be extracted");
    } else if (file_extension == "txt") // TXT files
        return BuildResponse(reply::ok, content + "\r\n");
    else
        return BuildResponse(reply::not_implemented, "File type is not supported by server\r\n");
    
}
std::string TextViewHandler::render_markdown(const std::string& content) {
    std::istringstream stream(content);
    
    // for parsing thru lists
    bool parsing_list = false;
    std::string list_tag = "";
    
    std::string line;
    std::stringstream response;
    response << "<!DOCTYPE html>\n<html>\n";
    // read each line in markdown
    while (std::getline(stream, line))
        response << convert_to_html(line, parsing_list, list_tag);
    response << add_close_tag(list_tag, parsing_list) << "</html>\r\n";
    return response.str();
}
bool TextViewHandler::read_pdf(std::string& content, const std::string &id) {
    std::string pdf_filepath =  view_dir_ + "/" + id;
    std::string txt_filepath =  view_dir_ + "/" + id + ".txt";
    // prevent shell injections
    boost::smatch match;
    boost::regex id_pattern(R"(^([a-zA-Z0-9_\-\.]+).pdf$)");
    if (!boost::regex_match(id, match, id_pattern))
        return false;
    // Convert PDF to txt via command line: pdftotext id.pdf id.pdf.txt
    std::string pdftotext_command = "pdftotext " + pdf_filepath + " " + txt_filepath;
    int failure = system(pdftotext_command.c_str()); // returns 0 on success
    if (failure)
        return false;
    // extract text from id.pdf.txt 
    if (!read_file(id + ".txt", content))
        return false;
    // Conversion adds trailing \n\n\f so remove it
    std::regex newline_regex(R"((.*)\n\n\f$)");
    content = std::regex_replace(content, newline_regex, "$1");
    content += "\r\n";
    // remove created txt file
    std::string cleanup_command = "rm " + txt_filepath;
    failure = system(cleanup_command.c_str()); 
    if (failure)
        return false;
    return true;
}
bool TextViewHandler::parse_uri(const std::string& uri, std::string& id) {
    boost::smatch match;
    boost::regex id_pattern(R"(^\/view\/(.+)$)");
    if (!boost::regex_match(uri, match, id_pattern)) // invalid uri path
        return false;
    id = match[1];
    if (id.find("..") != std::string::npos || id.front() == '/') // prevent path attacks
        return false;
    return true;
}
std::string TextViewHandler::urlDecode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.length());
    
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            // Extract the two hex digits after %
            std::string hex = encoded.substr(i + 1, 2);
            
            // Convert hex to integer
            int value;
            std::stringstream ss;
            ss << std::hex << hex;
            ss >> value;
            
            // Add the decoded character
            decoded += static_cast<char>(value);
            
            // Skip the next two characters (hex digits)
            i += 2;
        } else if (encoded[i] == '+') {
            // + is often used to represent space in URL encoding
            decoded += ' ';
        } else {
            // Regular character, add as-is
            decoded += encoded[i];
        }
    }
    
    return decoded;
}
bool TextViewHandler::read_file(const std::string& id, std::string& file_content) {
    std::string filepath = view_dir_ + "/" + id;
    std::ifstream file(filepath);
    if (!file.is_open())
        return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file_content = buffer.str();
    file.close();
    return true;
}
bool TextViewHandler::parse_file_extension(const std::string& id, std::string& file_extension) {
    boost::smatch match;
    boost::regex file_extension_pattern(R"(^(.+)\.(.+)$)");
    if (!boost::regex_match(id, match, file_extension_pattern)) // Failed to match file extension
        return false;
    
    file_extension = match[2];
    return true;
}
std::string TextViewHandler::convert_to_html(const std::string& line, bool& parsing_list, std::string& tag) {
    boost::smatch match;
    std::stringstream response;
    boost::regex header_regex_pattern(R"(^#{1,6}\s+(.*)$)");
    boost::regex ordered_list_regex_pattern(R"(^\t*\d\.\s+(.*)$)");
    boost::regex unordered_list_regex_pattern(R"(^\t*[\*\-\+]\s+(.*)$)");
    if (boost::regex_match(line, match, header_regex_pattern)) {
        // Header
        response << add_close_tag(tag, parsing_list); // end of list so add close tag
        int num = find_header_level(line);
        response << "<h" << num << ">" << match[1] << "</h" << num << ">\n";
    } else if (boost::regex_match(line, match, ordered_list_regex_pattern)) {
        // Ordered List
        response << add_open_tag(tag, "ol", parsing_list); // start of list so add open tag
        response << "<li>" << match[1] << "</li>\n";
    } else if (boost::regex_match(line, match, unordered_list_regex_pattern)) {
        // Unordered List
        response << add_open_tag(tag, "ul", parsing_list); // start of list so add open tag
        response << "<li>" << match[1] << "</li>\n";
    } else {
        // Assume paragraph if not matched
        if (line.empty())
            return "";
        
        response << add_close_tag(tag, parsing_list); // end of list  so add close tag
        response << "<p>" << handle_paragraph(line) << "</p>\n";
    }
    return response.str();
}
int TextViewHandler::find_header_level(const std::string& header) {
    int header_num = 0;
    boost::regex whitespace(R"(\s)");
    boost::smatch match;
    for (int i = 0; i < header.length(); i++) {
        if (boost::regex_match(header.substr(i, 1), match, whitespace)) // encountered end of #
            return header_num;
        header_num++;
    }
    return header_num;
}
std::string TextViewHandler::add_open_tag(std::string& tag, const std::string& tag_name, bool& status) {
    if (!status) {
        status = true;
        tag = tag_name;
        return "<" + tag_name + ">\n";
    }
    return "";
}
std::string TextViewHandler::add_close_tag(const std::string& tag, bool& status) {
    if (status) {
        status = false;
        return "</" + tag + ">\n";
    }
    return "";
}
std::string TextViewHandler::handle_paragraph(const std::string& line) {
    std::string paragraph = line;
    // Bold
    std::regex bold_regex(R"((\*\*|__)(.*?)\1)");
    std::string bold_html = "<strong>$2</strong>";
    paragraph = std::regex_replace(line, bold_regex, bold_html);
    // Italics
    // note: Order of replacement needs to be Bold -> Italics because  
    //       italic_regex also matches bold_regex
    std::regex italics_regex(R"((\*|_)(.*?)\1)");
    std::string italics_html = "<em>$2</em>";
    paragraph = std::regex_replace(paragraph, italics_regex, italics_html);
    // Link: Hyperlink
    std::regex link_regex(R"((\[(.*)\])?\(?(https?:\/\/[^\s\)\(]+\.[^\s\)\(]+)\)?)");
    std::string link_html = "<a href=\"$3\">$2</a>";
    paragraph = std::regex_replace(paragraph, link_regex, link_html);
    // Add URL as hyperlink if hyperlink field is empty
    std::regex href_tag_regex(R"((<a href=\"(.*)\">)(<\/a>))");
    std::string href_tag_html = "<a href=\"$2\">$2</a>";
    paragraph = std::regex_replace(paragraph, href_tag_regex, href_tag_html);
    
    return paragraph;
}
bool TextViewHandler::Register() {
    std::cout << "Registering TextViewHandler" << std::endl;
    return RequestHandlerRegistry::RegisterHandler("TextViewHandler", TextViewHandler::Init);
}
  
static struct TextViewHandlerRegistrar {
    TextViewHandlerRegistrar() {
        std::cout << "TextViewHandlerRegistrar constructor called" << std::endl;
        TextViewHandler::Register();
    }
} textViewHandlerRegistrar;
} // namespace server
} // namespace http