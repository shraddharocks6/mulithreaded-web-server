#include "http_server.hh"
#include <vector>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

vector<string> split(const string &s, char delim) {
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim)) {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}

HTTP_Request::HTTP_Request(string request) {
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

   //TODO : extract the request method and URL from first_line here
  this->method = first_line[0];
  this->url = first_line[1];

  if (this->method != "GET") {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}

HTTP_Response *handle_request(string req) {
  HTTP_Request *request = new HTTP_Request(req);

  HTTP_Response *response = new HTTP_Response();

  string url = string("html_files") + request->url;

  response->HTTP_version = "1.0";

  struct stat sb;
  if (stat(url.c_str(), &sb) == 0) // requested path exists
  {
    response->status_code = "200";
    response->status_text = "OK";
    response->content_type = "text/html";
    int flag = 0;
    string body;

    if (S_ISDIR(sb.st_mode)) {
      /*
      In this case, requested path is a directory.
      TODO : find the index.html file in that directory (modify the url
      accordingly)
      */
      struct stat buffer;
      string check = url +"index.html";
      int exist = stat(check.c_str(), &buffer);
      if(exist == 0){
        url = check;
      }
      else{
        response->status_code = "404";
        response->status_text = "Not Found";
        response->msg = "<h1>Error 404: Not Found</h1>\n";
        flag = 1;
      }

    }

    /*
    TODO : open the file and read its contents
    */
    if(!flag){
      string text;
      ifstream fileread(url);
      while (getline (fileread, text)) {
        response->msg = response->msg + text + "\n";
      }
      fileread.close();
    }

    /*
    TODO : set the remaining fields of response appropriately
    */
    response->status_text="OK";
    response->content_length=to_string((response->msg).size());
    response->content_type="text/html";  
  }

  else {
    /*
    TODO : set the remaining fields of response appropriately
    */
    response->status_code = "404";
    response->status_text = "Not Found";
    response->msg = "<h1>Error 404: Not Found</h1>\n";
    response->content_length=to_string((response->msg).size());
    response->content_type="text/html";
  }
  char date[100];
  time_t curr_time;
  tm * curr_tm;
  time(&curr_time);
  curr_tm = gmtime(&curr_time);  
  strftime(date, 100, "%a, %d %b %Y %T GMT", curr_tm);
  response->date = date;

  delete request;

  return response;
}

string HTTP_Response::get_string() {
  /*
  TODO : implement this function
  */
 this->body += ("HTTP/" + this->HTTP_version + " " + this->status_code + " " + this->status_text + "\n");
 this->body += ("Date: " + this->date + "\n");
 this->body += ("Content-Type: " + this->content_type + "\n");
 this->body += ("Content-Length: " + this->content_length + "\n");
 this->body += ("\n\n");
 this->body += (this->msg);
 return this->body;
}
